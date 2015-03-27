#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#define DEBUG
#include "debug.h"
#include "terminal.h"
#include "generador.h"
#include "basicgen.h"

/** Valor externo para NURand proveniente de generador.o */
extern int CValue;

void baraja(int *,int);
void term_nuevo_pedido     (FILE *);
void term_pago             (FILE *);
void term_estado_pedido    (FILE *);
void term_envio            (FILE *);
void term_nivel_existencias(FILE *);

void terminal_tpcc(FILE *fich,uint64_t numTran) {
	int tipos_transaccion[BARAJA_LEN]=BARAJA_CARTAS;
	uint64_t i;

	DEBUGF("[terminal_tpcc](FILE:%p, numTran:%llu)\n",fich,numTran);
	
	
	for(i=0;i<numTran;i++) {
		/* barajar transacciones cada cierto tiempo */
		if ((i%BARAJA_LEN) == 0) baraja(tipos_transaccion,BARAJA_LEN);
			
		switch(tipos_transaccion[i%BARAJA_LEN]) {
			case 0: DEBUGF("[terminal_tpcc] Transaccion de Nuevo Pedido (%llu/%llu)\n",i,numTran-1);
				term_nuevo_pedido(fich); 
				break;
			case 1: DEBUGF("[terminal_tpcc] Transaccion de Pago (%llu/%llu)\n",i,numTran-1);
				term_pago(fich);
				break;
			case 2: DEBUGF("[terminal_tpcc] Transaccion de Estado de Pedido (%llu/%llu)\n",i,numTran-1);
				term_estado_pedido(fich);
				break;
			case 3: DEBUGF("[terminal_tpcc] Transaccion de Envio (%llu/%llu)\n",i,numTran-1);
				term_envio(fich);
				break;
			case 4: DEBUGF("[terminal_tpcc] Transaccion de Nivel de Existencias (%llu/%llu)\n",
					i,numTran-1);
				term_nivel_existencias(fich);
				break;
			default:
				printf("[terminal_tpcc] Error. Esto no debiera de suceder (%llu/%llu)\n",i,numTran-1);
				fflush(NULL);
				exit(1);
				break;
		}
	}
	DEBUGS("[terminal_tpcc] ==== Finalizar");
}

/** Baraja un vector de elementos enteros */
void baraja(int *vect,int max) {
	int i,tmp,a,b;
		
	/* Intercambiamos dos a dos */
	for(i=0;i<max;i++) {
		a=random()%max;
		b=random()%max;
		tmp=vect[a];
		vect[a]=vect[b];
		vect[b]=tmp;
	}
}

/** Simula los datos de un nuevo pedido */
void term_nuevo_pedido(FILE *fich) {
	uint32_t w_id,d_id,c_id,ol_cnt,ol_i_id,ol_supply_w_id,ol_quantity;
       	int i;
	
	DEBUGF("[term_nuevo_pedido] FILE:%p - Generando datos\n",fich);
        
        w_id=random()%CARD_ALMACEN;/* Seleccionar almacen */
        d_id=random()%CARD_ZONA;   /* Seleccionar zona */
        c_id=gen_NURand(1023,0,(CARD_CLIENTE/CARD_ZONA)-1);
        ol_cnt=gen_number(5,15);   /* De cuantos elementos consta el pedido */
        DEBUGF("[term_nuevo_pedido] w_id:%d d_id:%d c_id:%d ol_cnt:%d\n",w_id,d_id,c_id,ol_cnt);

	/* Guardar */
        if (fprintf(fich,DS_TERM_NUEVOPEDIDO,w_id,d_id,c_id,ol_cnt)<0) {
		perror("");
		fprintf(stderr,"[term_nuevo_pedido] ERROR No puedo escribir los datos del nuevo pedido\n");
		exit(1);
	}
        /* Para cada elemento del pedido */
        for(i=0;i<ol_cnt;i++) {
                ol_i_id=gen_NURand(8191,0,CARD_PRODUCTO-1); /* Que producto pedir */
                ol_supply_w_id=w_id;                      /* De que almacen cogerlo */
                /* Un 1% de los pedidos son externos */      
                if (!random()%100) {
                        ol_supply_w_id=random()%CARD_ALMACEN;
                        /* Si hemos vuelto a calcular el mismo */
                        if (ol_supply_w_id==w_id) ol_supply_w_id=(ol_supply_w_id+1)%CARD_ALMACEN;
                }       
                ol_quantity=gen_number(1,10); /* Cuantos productos queremos */
                DEBUGF("[term_nuevo_pedido] linea_pedido i_id:%u w_supply:%u quantity:%u\n",
                        ol_i_id,ol_supply_w_id,ol_quantity);
		/* Guardar Linea Pedido */
		if (fprintf(fich,DS_TERM_LINEAPEDIDO,ol_i_id,ol_supply_w_id,ol_quantity)<0) {
			perror("");
			fprintf(stderr,"[term_nuevo_pedido] ERROR No puedo escribir los datos del nuevo pedido");
			exit(1);
		}
        }	
}

void term_pago(FILE *fich) {
	uint32_t w_id,d_id,cliente_last,c_id,c_d_id,c_w_id,h_amount;
	time_t h_date;
	char c_last[17];

       /* 1) Generar datos de entrada */
        w_id=random()%CARD_ALMACEN;
        d_id=random()%CARD_ZONA;
        
        /* Cliente 40% ID - 60% apellido */
        if ((random()%100) > 59) {
                cliente_last=0;
                c_id=gen_NURand(1023,0,(CARD_CLIENTE/CARD_ZONA)-1);
		/* guardamos el identificador como cadena para asi tener una sola cadena de
		   escritura y lectura */
		snprintf(c_last,17,"%u",c_id);
        }                          
        else {                     
                cliente_last=1;
                gen_last(c_last,gen_NURand(255,0,999));
        }
        
        /* Almacen del cliente 85% home 15% remoto */
        if ((random()%100) > 84) {
                c_d_id=random()%CARD_ZONA;
                c_w_id=random()%CARD_ALMACEN;
                if(c_w_id == w_id) c_w_id=(c_w_id+1)%CARD_ALMACEN;
                if(c_d_id == d_id) c_d_id=(c_d_id+1)%CARD_ALMACEN;
        }
        else {
                c_d_id=d_id;                    
                c_w_id=w_id;
        }

        h_amount=gen_number(100,500000);
        h_date=time(NULL);
        DEBUGF("[term_pago] Generado en: w_id:%u d_id:%u - cantidad:%u fecha:%d\n",
                w_id,d_id,h_amount,h_date);
        #ifdef DEBUG
        if (cliente_last) DEBUGF("[term_pago] Cliente apellido:%s w_id:%u d_w:%u\n",c_last,c_w_id,c_d_id);
        else DEBUGF("[term_pago] Cliente id:%u w_id:%u d_w:%u\n",c_id,c_w_id,c_d_id);
        #endif
	
	/* 2) Volcado al fichero */
	if (fprintf(fich,DS_TERM_PAGO,w_id,d_id,cliente_last,c_last,c_d_id,c_w_id,h_amount,h_date)<0) {
		perror("");
		fprintf(stderr,"[term_pago] No puedo escribir los datos de pago\n");
		exit(1);
	}
}

/** Genera datos para la transaccion estado de pedido */
void term_estado_pedido(FILE *fich) {
	uint32_t w_id,d_id,cliente_last,c_id;
	char c_last[17];

        /* 1) Generar datos de entrada */
        w_id=random()%CARD_ALMACEN;
        d_id=random()%CARD_ZONA;

        /* Cliente 40% ID - 60% apellido */
        if ((random()%100) > 59) {
                cliente_last=0;
                c_id=gen_NURand(1023,0,(CARD_CLIENTE/CARD_ZONA)-1);
		snprintf(c_last,17,"%u",c_id);
        }
        else {
                cliente_last=1;
                gen_last(c_last,gen_NURand(255,0,999));
        }

        DEBUGF("[estado_pedido] Generado en: w_id:%u d_id:%u\n",w_id,d_id);
        #ifdef DEBUG
        if (cliente_last) DEBUGF("[estado_pedido] Cliente apellido:%s\n",c_last);
        else DEBUGF("[estado_pedido] Cliente id:%u\n",c_id);
        #endif

	/* 2) Volcado a fichero */
	if (fprintf(fich,DS_TERM_ESTADOPEDIDO,w_id,d_id,cliente_last,c_last)<0) {
		perror("");
		fprintf(stderr,"[term_estado_pedido] ERROR No puedo escribir los datos generados\n");
		exit(1);
	}
}

void term_envio(FILE *fich) {
	uint32_t w_id,o_carrier_id;
	time_t ol_delivery_d;
	
	/* 1) Generar datos de entrada */
	w_id=random()%CARD_ALMACEN;
	o_carrier_id=gen_number(1,10);
	ol_delivery_d=time(NULL);
	DEBUGF("[term_envio] w_id:%u o_carrier_id:%u ol_delivery_d:%u\n",w_id,o_carrier_id,ol_delivery_d);

	/* 2) Volcado a fichero */
	if (fprintf(fich,DS_TERM_ENVIO,w_id,o_carrier_id,ol_delivery_d)<0) {
		perror("");
		fprintf(stderr,"[term_envio] ERROR No puedo escribir los datos generados\n");
		exit(1);
	}
}

void term_nivel_existencias(FILE *fich) {
	uint32_t w_id,d_id,limite;
	
	/* 1) Generar datos de entrada */
	w_id=random()%CARD_ALMACEN;
	d_id=random()%CARD_ZONA;
	limite=gen_number(10,20);
	DEBUGF("[term_nivel_existencias] w_id:%u d_id:%u limite:%u\n",w_id,d_id,limite);

	/* 2) Volcado a fichero */
	if (fprintf(fich,DS_TERM_NIVELEXISTENCIAS,w_id,d_id,limite)<0) {
		perror("");
		fprintf(stderr,"[term_nivel_existencias] ERROR NO puedo escribir los datos generados\n");
		exit(1);
	}
}

