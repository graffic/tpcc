#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "basicgen.h"
#include "generador.h"
#include "registros.h"
#include "terminal.h"

/* Macros de ayuda */
/* Forma general de abrir un fichero */
#define ABRIR_FICHERO(estructura,nfich,modo) \
	snprintf(nombre_fichero,300,"%s%s",prefijo_ficheros,nfich);\
	if ((estructura=fopen(nombre_fichero,modo))==NULL) {\
		perror("");\
		fprintf(stderr,"No puedo abrir el fichero: %s\n",nombre_fichero); \
		exit(2);\
	}\
	
/* Globales */
/** Constante C para la funcion-macro NURand */
int CValue;
/** Prefijo para TODOS los ficheros de datos */
char prefijo_ficheros[205];
/** Nombre de fichero que variará dependiendo de la funcion de generacion */
char nombre_fichero[300];

/* Generadores generales */
void generarProductos(int);
void generarAlmacenes(uint32_t);
void generarExistencias(int,RegAlmacen *);
void generarZonas(int,RegAlmacen *);
void generarClientes(int,RegZona *);
void generarPedidos(int,RegZona *);
void generarLineasPedido(RegPedido *);
void generarNuevosPedidos(int,int,RegZona *);
void generarConstantes(uint32_t,uint64_t);

void generarConstantes(uint32_t numServ,uint64_t numTran) {
	FILE *fconst;
	
	/* Abrimos el fichero donde guardar las constantes */
	ABRIR_FICHERO(fconst,FICHERO_CONSTANTES,"w")

	/* Almacenar en disco */
	if (fprintf(fconst,"%u %llu\n",numServ,numTran)<0) {
		perror("No puedo escribir en el fichero de constantes");
		exit(2);
	}
	
	/* Cerrar fichero */
	fclose(fconst);
}

void generarNuevosPedidos(int inicio,int cant,RegZona *rz) {
	FILE *fnp;
	RegNuevoPedido rnp;
	uint32_t i;
	char *fmodo="a";

	/* Modo de apertura */
	if (rz->d_id==0 & rz->d_w_id==0) fmodo="w";
	
	/* Abrimos el fichero donde volcar los nuevos pedidos generados */
	ABRIR_FICHERO(fnp,FICHERO_NUEVOSPEDIDOS,fmodo)

	/* Informacion extra */
	printf("-> Generando %d nuevos pedidos (partiendo de %d) para la zona Num %d del almacen Num %d\n",
	       cant,inicio,rz->d_id,rz->d_w_id);
	
	/* Generamos nuevos pedidos */
	rnp.no_w_id=rz->d_w_id;
	rnp.no_d_id=rz->d_id;
	for(i=inicio;i<(inicio+cant);i++) {
		/* Generar nuevo pedido */
		rnp.no_o_id=i;
		/* Almacenar nuevo pedido */
		if(fprintf(fnp,DUMPSTRING_NUEVOPEDIDO,DUMPPARAM_NUEVOPEDIDO(rnp))<0) {
			perror("No puedo escribir en el fichero de nuevos pedidos");
			exit(2);
		}
	}

	/* Cerramos el fichero */
	fclose(fnp);
}

void generarLineasPedido(RegPedido *rp) {
	FILE *flps;
	RegLineaPedido rlp;
	uint32_t i;
	char *fmodo="a";
	
	/* Modo de apertura */
	if(rp->o_id==0 && rp->o_d_id==0 && rp->o_w_id==0) fmodo="w";

	/* Abrimos el fichero donde volcar los clientes generados */
	ABRIR_FICHERO(flps,FICHERO_LINEASPEDIDO,fmodo)

	/* Generar y almacenar lineas de pedido */
	rlp.ol_o_id=rp->o_id;
	rlp.ol_d_id=rp->o_d_id;
	rlp.ol_w_id=rp->o_w_id;
	rlp.ol_supply_w_id=rp->o_w_id;
	for(i=0;i<rp->o_ol_cnt;i++) {
		/* Generar linea de pedido */
		rlp.ol_number=i;
		rlp.ol_i_id=gen_number(0,CARD_PRODUCTO-1);
		if(rlp.ol_o_id< ((CARD_PEDIDO-CARD_NUEVOPEDIDO)/CARD_ZONA) ) {
			rlp.ol_delivery_d=rp->o_entry_d;
			rlp.ol_amount=0;
		}
		else {
			rlp.ol_delivery_d=0;
			rlp.ol_amount=gen_number(1,9999999);
		}
		rlp.ol_quantity=5;
		gen_a_string(rlp.ol_dist_info,24,24);
		/* Guardar linea de pedido */
		if(fprintf(flps,DUMPSTRING_LINEAPEDIDO,DUMPPARAM_LINEAPEDIDO(rlp))<0) {
			perror("No puedo escribir en el fichero de lineas de pedido");
			exit(2);
		}			
	}
	
	/* Cerrar fichero */
	fclose(flps);
}

void generarPedidos(int num,RegZona *rz) {
	FILE      *fped;
	RegPedido    rp;
	uint32_t   i,j;
	char *fmodo="a";
	char *clientes;
	
	/* Modo de apertura */
	if (rz->d_id==0 && rz->d_w_id==0) fmodo="w";
	
	/* Abrimos el fichero donde volcar los clientes generados */
	ABRIR_FICHERO(fped,FICHERO_PEDIDOS,fmodo)
	
	/* Informacion extra */
	printf("-> Generando %d pedidos para la zona Num %u del almacen Num %u\n",num,rz->d_id,rz->d_w_id);
	
	/* Inicializar pseudo-permutacion */
	clientes=(char*)malloc(sizeof(char)*num);
	bzero(clientes,sizeof(char)*num);
	
	/* Generar y almacenar pedidos */
	rp.o_w_id=rz->d_w_id;
	rp.o_d_id=rz->d_id;
	for(i=0;i<num;i++) {
		/* Generar Pedido */
		rp.o_id=i;
		
		/* Pseudo-permutacion para el id cliente */
		j=random()%num;
		if(clientes[j]==0) {
			clientes[j]=1;
			rp.o_c_id=j;
		}
		else {
			for(j=0;j<num && clientes[j];j++);
			if (j<num) clientes[j]=1,rp.o_c_id=j;
			else puts("ERROR necesito mas clientes y no tengo");
		}
		
		rp.o_entry_d=time(NULL);
		if(rp.o_id< ((CARD_PEDIDO-CARD_NUEVOPEDIDO)/CARD_ZONA) ) rp.o_carrier_id=gen_number(1,10);
		else rp.o_carrier_id=0;
		rp.o_ol_cnt=gen_number(5,15);
		rp.o_all_local=1;
		/* Para el pedido actual, generar lineas de pedido */
		generarLineasPedido(&rp);
		/* Almacenar pedido */
		if(fprintf(fped,DUMPSTRING_PEDIDO,DUMPPARAM_PEDIDO(rp))<0) {
			perror("No puedo escribir en el fichero de pedidos");
		}
	}
	
	/* Cerramos el fichero */
	fclose(fped);
	free(clientes);
}

void generarClientes(int num,RegZona *rz) {
	FILE      *fcli,*fhis;
	RegCliente   rc;
	RegHistorico rh;
	uint32_t     i;
	char *fmodo="a";

	/* Modo de apertura */
	if (rz->d_id==0 && rz->d_w_id==0) fmodo="w";

	/* Abrimos el fichero donde volcar los clientes generados */
	ABRIR_FICHERO(fcli,FICHERO_CLIENTES,fmodo)
	
	/* Abrimos donde volcar el historico */
	ABRIR_FICHERO(fhis,FICHERO_HISTORICO,fmodo)

	/* Informacion extra */
	printf("-> Generando %d clientes para la zona Num %u del almacen Num %u\n",num,rz->d_id,rz->d_w_id);

	/* Generar y almacenar clientes */
	rc.c_w_id=rz->d_w_id;
	rc.c_d_id=rz->d_id;
	/* Datos para el historico del cliente */
	rh.h_c_d_id = rh.h_d_id = rz->d_id; 
	rh.h_c_w_id = rh.h_w_id = rz->d_w_id;
	for(i=0;i<num;i++) {
		/* Generar un cliente */
		rc.c_id=i;
		gen_a_string(rc.c_first,8,16);
		memcpy(rc.c_middle,"OE",3);
		if (i<1000) gen_last(rc.c_last,i);
		else gen_last(rc.c_last,gen_NURand(255,0,999));
		gen_a_string(rc.c_street_1,10,20);
		gen_a_string(rc.c_street_2,10,20);
		gen_a_string(rc.c_city,10,20);
		gen_a_string(rc.c_state,2,2);
		gen_zip(rc.c_zip);
		gen_n_string(rc.c_phone,16,16);
		rc.c_since=time(NULL);
		if(random()%10) memcpy(rc.c_credit,"BC",3);
		else memcpy(rc.c_credit,"GC",3);
		rc.c_credit_lim=5000000;
		rc.c_discount=gen_number(0,5000);
		rc.c_balance=-1000;
		rc.c_ytd_payment=10000;
		rc.c_payment_cnt=1;
		rc.c_delivery_cnt=0;
		gen_a_string(rc.c_data,300,500);
		/* Generar un historico */
		rh.h_c_id   = rc.c_id;
		rh.h_date=time(NULL);
		rh.h_amount=1000;    
		gen_a_string(rh.h_data,12,24);
		/* Guardar un cliente */
		if(fprintf(fcli,DUMPSTRING_CLIENTE,DUMPPARAM_CLIENTE(rc))<0) {
                        perror("No puedo escribir en el fichero de clientes");
			exit(2);
		}
		/* Guardar historico */
		if(fprintf(fhis,DUMPSTRING_HISTORICO,DUMPPARAM_HISTORICO(rh))<0) {
			perror("No puedo escribir en el fichero del historico");
			exit(2);
		}
	}
	
	/* Cerramos los ficheros */
	fclose(fcli);
	fclose(fhis);
}

void generarExistencias(int num,RegAlmacen *ra) {
	FILE *fext;
	RegExistencias re;
	uint32_t i;
	char *fmodo="a";
	
	/* Modo de apertura */
	if (ra->w_id==0) fmodo="w";

	/* Abrimos el fichero donde volcar las existencias generadas */
	ABRIR_FICHERO(fext,FICHERO_EXISTENCIAS,fmodo)
	
	/* Informacion extra */
	printf("-> Generando %d entradas de stock para el almacen Num %u\n",num,ra->w_id);
	
	/* Generar y almacenar existencias */
	re.s_w_id=ra->w_id;
	for(i=0;i<num;i++) {
		/* Generar una existencia */
		re.s_i_id=i;
		re.s_quantity=gen_number(10,100);
		gen_a_string(re.s_dist_01,24,24);
		gen_a_string(re.s_dist_02,24,24);
		gen_a_string(re.s_dist_03,24,24);
		gen_a_string(re.s_dist_04,24,24);
		gen_a_string(re.s_dist_05,24,24);
		gen_a_string(re.s_dist_06,24,24);
		gen_a_string(re.s_dist_07,24,24);
		gen_a_string(re.s_dist_08,24,24);
		gen_a_string(re.s_dist_09,24,24);
		gen_a_string(re.s_dist_10,24,24);
		re.s_ytd=0;
		re.s_order_cnt=0;
		re.s_remote_cnt=0;
		gen_a_string(re.s_data,26,50);
		if((random()%10)==0) memcpy(re.s_data,"ORIGINAL",8);
		/* Almacenar la existencia creada */
		if(fprintf(fext,DUMPSTRING_EXISTENCIAS,DUMPPARAM_EXISTENCIAS(re))<0) {
			perror("No puedo escribir en el fichero de almacenes");
			exit(2);
		}
	}
	
	/* Cerramos el fichero */
	fclose(fext);
}

void generarZonas(int num,RegAlmacen *ra) {
	FILE *fzon;
	RegZona rz;
	uint32_t i,tmp;
	char *fmodo="a";

        /* Modo de apertura */
        if (ra->w_id==0) fmodo="w";

        /* Abrimos el fichero donde volcar las zonas generadas */
        ABRIR_FICHERO(fzon,FICHERO_ZONAS,fmodo)

	/* Informacion extra */
	printf("-> Generando %d zonas para el almacen Num %u\n",num,ra->w_id);
	
        /* Generar y almacenar zonas */
        rz.d_w_id=ra->w_id;
	for(i=0;i<num;i++) {
		/* Generar la zona */
		rz.d_id=i;
		gen_a_string(rz.d_name,6,10);
		gen_a_string(rz.d_street_1,10,20);
		gen_a_string(rz.d_street_2,10,20);
		gen_a_string(rz.d_city,10,20);
		gen_a_string(rz.d_state,2,2);
		gen_zip(rz.d_zip);
		rz.d_tax=gen_number(0,2000);
		rz.d_ytd=3000000;
		rz.d_next_o_id=CARD_PEDIDO/num;
		/* Generar Clientes */
		generarClientes(CARD_CLIENTE/num,&rz);
		/* Generar Pedidos */
		generarPedidos(CARD_PEDIDO/num,&rz);
		/* Generar Nuevos pedidos */
		tmp=CARD_NUEVOPEDIDO/num;
		generarNuevosPedidos(CARD_PEDIDO/num-tmp,tmp,&rz);
		/* Guardar zona */
		if(fprintf(fzon,DUMPSTRING_ZONA,DUMPPARAM_ZONA(rz))<0) {
			perror("No puedo escribir en el fichero de zonas");
			exit(2);
		}
	}

	/* Cerramos el fichero */
	fclose(fzon);
}

void generarAlmacenes(uint32_t num) {
	FILE *falm;
	RegAlmacen ra;
	uint32_t i;

	/* Abrimos el fichero donde volcar los almacenes generados */
	ABRIR_FICHERO(falm,FICHERO_ALMACENES,"w")

	/* Informacion extra */
	printf("-> Generando %u almacenes\n",num);

	/* Generar y guardar todos los almacenes asi como lanzar
	   los procesos necesarios para generar todo lo que depende
	   del numero de almacenes */
	for(i=0;i<num;i++) {
		/* Generar almacen */
		ra.w_id=i;
        	gen_a_string(ra.w_name,6,11);
        	gen_a_string(ra.w_street_1,10,20);
        	gen_a_string(ra.w_street_2,10,20);
        	gen_a_string(ra.w_city,10,20);
        	gen_a_string(ra.w_state,2,2);
        	gen_zip(ra.w_zip);
        	ra.w_tax=gen_number(0,2000);
        	ra.w_ytd=30000000;
		/* Por cada almacen hay CARD_EXISTENCIAS existencias */
		generarExistencias(CARD_EXISTENCIAS,&ra);
		/* Por cada almacen hay CARD_ZONA zonas */
		generarZonas(CARD_ZONA,&ra);
		/* Volcarlo a disco */
		if(fprintf(falm,DUMPSTRING_ALMACEN,DUMPPARAM_ALMACEN(ra))<0) {
			perror("No puedo escribir en el fichero de almacenes");
			exit(2);
		}
	}

	/* Cerrar fichero */
	fclose(falm);
						
}

void generarProductos(int num) {
	FILE *fprod;
	RegProducto rp;
	uint32_t i;

	/* Abrimos el fichero donde volcar los prodcutos generados */
	ABRIR_FICHERO(fprod,FICHERO_PRODUCTOS,"w")

	/* Informacion extra */
	printf("-> Generando %d productos\n",num);
	
	/* Generar y almacenar todos los productos */
	for(i=0;i<num;i++) {
		/* Generar un producto */
		rp.i_id=i;
		rp.i_im_id=gen_number(0,10000);
		gen_a_string(rp.i_name,14,24);
		rp.i_price=gen_number(100,10000);
		gen_a_string(rp.i_data,26,50);
		if((random()%10)==0) memcpy(rp.i_data,"ORIGINAL",8);
		/* Volcarlo a disco */
		if (fprintf(fprod,DUMPSTRING_PRODUCTO,DUMPPARAM_PRODUCTO(rp))<0) {
			perror("No puedo escribir en el fichero de productos");
			exit(2);
		};
	}
	
	/* Cerrar fichero */
	fclose(fprod);
}

void obtenerParametros(uint32_t *alm,uint32_t *srv,uint64_t *trans,char *exp,int argc,char **argv) {
	int ch,ayudaysalir=0,nopoblado=0;
	
	while((ch=getopt(argc,argv,"s:a:t:hc")) != -1) {
		switch(ch) {
			case 'a':
				sscanf(optarg,"%u",alm);
				if (*alm<1) *alm=CARD_ALMACEN;
				break;
			case 't':
				sscanf(optarg,"%llu",trans);
				if (*trans<1) *trans=CARD_TRANSACCION;
				break;
			case 's':
				sscanf(optarg,"%u",srv);
				if (*srv<1) *srv=CARD_SERVIDOR;
				break;
			case 'c':
				nopoblado=1;
				break;
			case 'h':
			default:
				ayudaysalir=1;
		}
	}

	/* Si al terminal aun quedan argumentos */
	if (optind == (argc-1)) {
		strncpy(exp,argv[optind],100);
	}
	/* Es obligatorio el directorio, si no esta, salir */
	else ayudaysalir=1;
	if (nopoblado) *alm=0;
	
	/* Mensaje de uso */
	if (ayudaysalir) {
		printf("Modo de uso:\n\t%s [<opciones>] <dir>\n\n",argv[0]);
		printf("Opciones:\n"
		       "\t-h         Este mensaje.\n"
		       "\t-c         No genera el poblado de la BD (Util para cambiar la carga)\n"
		       "\t-a numero  Cantidad de almacenes................(Por defecto: %u)\n"
		       "\t-t numero  Cantidad total de transacciones......(Por defecto: %llu)\n"
		       "\t-s numero  Cantidad de procesos servidor........(Por defecto: %u)\n"
		       "\t<dir>      Directorio destino del experimento...(Obligatorio)\n",
		       CARD_ALMACEN,(uint64_t)CARD_TRANSACCION,CARD_SERVIDOR);
		exit(1);
	}

}

void generarCarga (uint32_t numServ,uint64_t numTran) {
	uint32_t i;
	int CRun,CDelta;
	uint64_t tot,transerv=numTran/numServ;
	char nfich[20];
	FILE *fcarga;

	/* Ajustamos la constante CValue a CRun */
	do {
		CRun=gen_number(0,255);
		CDelta=abs(CRun-CValue);
	/* Debe de cumplir "ciertas" condiciones para que sea válido */	
	} while (CDelta==96 || CDelta==112 || CDelta<65 || CDelta>119);
	CValue=CRun;
	
	for(i=0;i<numServ;i++) {
		snprintf(nfich,20,"carga%u.txt",i);
		ABRIR_FICHERO(fcarga,nfich,"w")
		if (i==(numServ-1)) tot=transerv+numTran%numServ;
		else tot=transerv;
		
		printf("-> Generando Carga para el servidor %u - transaciones %llu\n",i,tot);
		terminal_tpcc(fcarga,tot);
		fclose(fcarga);
	}	
}

void inicializar(char *experimento) {
	int res;
	
	/* Directorio de destino */
	res=mkdir(experimento,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

	/* Hubo un problema y ese problema no es que el directorio ya exista */
	if (res && errno!=EEXIST) {
		perror("");
		fprintf(stderr,"ERROR: No se pudo crear el directorio: \"%s\"\n",experimento);
		exit(1);
	}
	/* Hubo un problema y es que el directorio existe - borrar carga */
	else if (res) {
		DIR           *direc;
		struct dirent *ds;
		char carga[300];
		int len;
		
		printf("-> El directorio \"%s\" existe, borrando carga.\n",experimento);
		/* Abrimos el directorio en busca de carga */
		direc=opendir(experimento);
		if(!direc) {
			perror("");
			fprintf(stderr,"No puedo abrir el directorio: \"%s\"\n",experimento);
			exit(1);
		}

		/* Buscamos carga */
		len=snprintf(carga,300,"%s_carga",experimento);
		/* Cambiamos de directorio para hace un unlink mas facilmente */
		chdir(experimento);
		while((ds=readdir(direc))!=NULL) {
			res=strncmp(carga,ds->d_name,len);
			if (res==0) {
				res=unlink(ds->d_name);
				printf("\t * Borrando %s (%d)\n",ds->d_name,res);
			}
		}
		closedir(direc);
		chdir("..");
	}

	/* Nombre de fichero */
	snprintf(prefijo_ficheros,202,"%s/%s_",experimento,experimento);
	
	/* Constante para NURand */
	CValue=gen_number(0,255);
}

int main(int argc,char **argv) {
	uint32_t numAlmacenes=CARD_ALMACEN;
	uint32_t numServidores=CARD_SERVIDOR;
	uint64_t numTransacciones=CARD_TRANSACCION;
	char experimento[100];
	
	/* Inicializar sistema aleatorio */
	srandom(time(NULL));
	
	/* Procesado de parametros */
	obtenerParametros(&numAlmacenes,&numServidores,&numTransacciones,experimento,argc,argv);
	
	
	/* Informacion de incio */
	printf("Ejecutando %s - Generando carga para \n"
	       "\t* %u Almacenes\n"
	       "\t* %llu Transacciones\n"
	       "\t* %u Servidores\n"
	       "\t* Directorio de salida: %s\n\n",
		argv[0],numAlmacenes,numTransacciones,numServidores,experimento);
	
	/* Preparar directorio destino */
	inicializar(experimento);
	
	/* Generar constantes */
	generarConstantes(numServidores,numTransacciones);
	
	/* La señal para generar poblado es que numAlmacenes sea distinto de 0 */
	if (numAlmacenes>0) {
		/* Generar aquello que no depende de los almacenes */
		generarProductos(CARD_PRODUCTO);	

		/* Generar almacenes y lo que dependa de almacenes */
		generarAlmacenes(numAlmacenes);
	
	}
	
	/* Generar carga */
	generarCarga(numServidores,numTransacciones);
	return 0;
}
