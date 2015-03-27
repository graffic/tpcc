#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
//#define DEBUG
#include "generador.h"
#include "cargador.h"
#include "debug.h"
#include "registros.h"
#include "arbolbmas.h"
#include "listaenlazada.h"
#include "terminal.h"

/** Valor necesario para la macro gen_NURand */
int CValue;

/** Transaccion de nuevo pedido.
    Ejecuta la transaccion de nuevo pedido tal y como se especifica en la seccion
    2.4 del estandar tpc-c
*/
void incrementa_d_next_o_id(RegZona *,RegZona *);
void calcula_existencias   (RegExistencias *,RegExistencias *);
void nuevo_pedido(BBDD *bd,FILE *fentrada) {
	/* Variables necesarias */
	uint32_t ol_i_id[15],ol_supply_w_id[15],ol_quantity[15];
	uint32_t i,res,cantidad_total=0;
	char brand_generic;
	/* Registros a usar */
	RegAlmacen alm;
	RegZona    zon;
	RegCliente cli;
	RegPedido  ped;
	RegNuevoPedido nped;
	RegProducto    pro;
	RegExistencias exi;
	RegLineaPedido lped;
	
	/* 1) Recuperar datos de entrada */
	DEBUGF("[nuevo_pedido] bd:%p - Recuperando datos \n",bd);
	/* Leer del fichero de entrada */
	if (fscanf(fentrada,RS_TERM_NUEVOPEDIDO,&alm.w_id,&zon.d_id,&cli.c_id,&ped.o_ol_cnt)<1) {
		perror("");
		fprintf(stderr,"[nuevo_pedido] No puedo leer del fichero de entrada el nuevo pedido\n");
		return;
	}
	
	DEBUGF("[nuevo_pedido] w_id:%d d_id:%d c_id:%d ol_cnt:%d\n",
		alm.w_id,zon.d_id,cli.c_id,ped.o_ol_cnt);
	
	/* Para cada elemento del pedido */
	for(i=0;i<ped.o_ol_cnt;i++) {
		/* Leer del fichero de entrada */
		if (fscanf(fentrada,RS_TERM_LINEAPEDIDO,ol_i_id+i,ol_supply_w_id+i,ol_quantity+i)<1) {
			perror("");
			fprintf(stderr,"[nuevo_pedido] No puedo leer del fichero las lineas del pedido\n");
			return;
		}
		DEBUGF("[nuevo_pedido] linea_pedido i_id:%u w_supply:%u quantity:%u\n",
			ol_i_id[i],ol_supply_w_id[i],ol_quantity[i]);
	}

	/* 2) Transaccion */
	/* Dado un almacen (w_id), distrito (d_w_id y d_id), cliente (c_w_id, c_d_id, c_id), 
	   cantidad de elementos (ol_cnt); y por cada elemento, el id del producto (ol_i_id),
	   almacen proveedor (ol_supply_w_id) y una cantidad (ol_quantity). */
	zon.d_w_id=alm.w_id;
	cli.c_w_id=alm.w_id;
	cli.c_d_id=zon.d_id;
	/* 2.1) The row in the WAREHOUSE table with matching W_ID is selected and W_TAX, 
                the warehouse tax rate, is retrieved. */
	res=abm_buscar(bd->almacenes,&alm);
	if(res==ABM_404) {
		DEBUGF("[nuevo_pedido] ERROR No se encontró el almacen %u\n",alm.w_id);
		return;
	}
	DEBUGF("[nuevo_pedido] Almacen:%u w_tax:%u\n",alm.w_id,alm.w_tax);

	/* 2.2) The row in the DISTRICT table with matching D_W_ID and D_ ID is selected, 
	        D_TAX, the district tax rate, is retrieved, and D_NEXT_O_ID, the next available 
		order number for the district, is retrieved and incremented by one. */
	res=abm_buscar(bd->zonas,&zon);
	if(res==ABM_404) {
		DEBUGF("[nuevo_pedido] ERROR No se encontró el distrito %d w:%d\n",zon.d_id,alm.w_id);
		return;
	}
	DEBUGF("[nuevo_pedido] Zona: %u (w:%u) d_tax:%u d_next_o_id:%u\n",
		zon.d_id,zon.d_w_id,zon.d_tax,zon.d_next_o_id);
	abm_modificar(bd->zonas,&zon,(doFunc)incrementa_d_next_o_id);
	DEBUGF("[nuevo_pedido] Nuevo d_next_o_id:%u\n",zon.d_next_o_id);
	
	/* 2.1) The row in the CUSTOMER table with matching C_W_ID, C_D_ID, and C_ID is selected 
	        and C_DISCOUNT, the customer's discount rate, C_LAST, the customer's last name, 
		and C_CREDIT, the customer's credit status, are retrieved. */
	res=abm_buscar(bd->clientes,&cli);
	if(res==ABM_404) {
		DEBUGF("[nuevo_pedido] ERROR No se encontró el cliente: %u (d:%u w:%u)\n",cli.c_id,zon.d_id,alm.w_id);
		return;
	}
	
	/* 2.3) A new row is inserted into both the NEW-ORDER table and the ORDER table to 
	        reflect the creation of the new order. O_CARRIER_ID is set to a null value. If 
		the order includes only home order-lines, then O_ALL_LOCAL is set to 1, otherwise 
		O_ALL_LOCAL is set to 0. */
	/* 2.4) The number of items, O_OL_CNT, is computed to match ol_cnt. */
	nped.no_o_id=ped.o_id=zon.d_next_o_id-1;
	nped.no_d_id=ped.o_d_id=zon.d_id;
	nped.no_w_id=ped.o_w_id=alm.w_id;
	ped.o_c_id=cli.c_id;
	DEBUGF("[nuevo_pedido] Nuevo pedido - o_id:%u d_id:%u w_id:%u\n",nped.no_o_id,nped.no_d_id,nped.no_w_id);
	ped.o_carrier_id=0;
	ped.o_all_local=1;
	/* Mirar si todas las lineas de pedido son locales */
	for(i=0;i<ped.o_ol_cnt;i++) { 
		if(ol_supply_w_id[i]!=alm.w_id) {
			ped.o_all_local=0;
			break;
		}
	}
	ped.o_entry_d=time(NULL);		
	DEBUGF("[nuevo_pedido] Pedido - id:%u d:%u w:%u - carrier:%u al:%u cnt:%u time:%u\n",
		ped.o_id,ped.o_d_id,ped.o_w_id,ped.o_carrier_id,ped.o_all_local,ped.o_ol_cnt,ped.o_entry_d);
	abm_insertar(bd->nuevospedidos,&nped);
	abm_insertar(bd->pedidos,&ped);
	
	/* 2.5) For each O_OL_CNT item on the order: */
	for(i=0;i<ped.o_ol_cnt;i++) {
			
	/* 2.5.1) The row in the ITEM table with matching I_ID (equals OL_I_ID) is selected and 
                  I_PRICE, the price of the item, I_NAME, the name of the item, and I_DATA are 
                  retrieved. */
		pro.i_id=ol_i_id[i];
		res=abm_buscar(bd->productos,&pro);
		if(res==ABM_404) {
			DEBUGF("[nuevo_pedido] ERROR No se encontró el producto %u\n",pro.i_id);
			return;
		}
		DEBUGF("[nuevo_pedido] (ol:%d) producto:%u i_price:%u i_name:\"%s\" i_data:\"%s\"\n",
			i,pro.i_id,pro.i_price,pro.i_name,pro.i_data);

	/* 2.5.2) The row in the STOCK table with matching S_I_ID (equals OL_I_ID) and S_W_ID 
	          (equals OL_SUPPLY_W_ID) is selected. S_QUANTITY, the quantity in stock, 
		  S_DIST_xx, where xx represents the district number, and S_DATA are retrieved. 
		  If the retrieved value for S_QUANTITY exceeds OL_QUANTITY by 10 or more, then 
		  S_QUANTITY is decreased by OL_QUANTITY; otherwise S_QUANTITY is updated to 
		  (S_QUANTITY - OL_QUANTITY)+91. S_YTD is increased by OL_QUANTITY and S_ORDER_CNT 
		  is incremented by 1. If the order-line is remote, then S_REMOTE_CNT is 
		  incremented by 1. */
		exi.s_i_id=pro.i_id;
		exi.s_w_id=ol_supply_w_id[i];
		
		/* Hay que buscar las existencias */
		abm_buscar(bd->existencias,&exi);
		DEBUGF("[nuevo_pedido] Datos viejos - s_quantity:%u s_ytd:%u s_order_cnt:%u s_remote_cnt:%u\n",
			exi.s_quantity,exi.s_ytd,exi.s_order_cnt,exi.s_remote_cnt);
		/* Preparacion para calcula_existencias */
		exi.s_quantity=ol_quantity[i];
		if(alm.w_id!=ol_supply_w_id[i]) exi.s_remote_cnt=1;
		else exi.s_remote_cnt=0;
		DEBUGF("[nuevo_pedido] linea - remota:%u cantidad:%u\n",exi.s_remote_cnt,exi.s_quantity);
		res=abm_modificar(bd->existencias,&exi,(doFunc)&calcula_existencias);
		if(res==ABM_404) {
			DEBUGF("[nuevo_pedido] ERROR No se encontro la existencia %u (w:%u)\n",exi.s_i_id,exi.s_w_id);
			return;
		}
		DEBUGF("[nuevo_pedido] Datos nuevos - s_quantity:%u s_ytd:%u s_order_cnt:%u s_remote_cnt:%u\n",
			exi.s_quantity,exi.s_ytd,exi.s_order_cnt,exi.s_remote_cnt);

	/* 2.5.3) The amount for the item in the order (OL_AMOUNT) is computed as: 
  		  OL_QUANTITY * I_PRICE */
		lped.ol_quantity=ol_quantity[i];
		lped.ol_amount=ol_quantity[i]*pro.i_price;
		cantidad_total+=lped.ol_amount;
		
	/* 2.5.4) The strings in I_DATA and S_DATA are examined. If they both include the string 
		  "ORIGINAL", the brand-generic field for that item is set to "B", otherwise, 
		  the brand-generic field is set to "G". */
		if (strncmp(pro.i_data,"ORIGINAL",8)==0 && strncmp(exi.s_data,"ORIGINAL",8)==0) brand_generic='B';
		else brand_generic='G';

	/* 2.5.5) A new row is inserted into the ORDER-LINE table to reflect the item on the 
		  order. OL_DELIVERY_D is set to a null value, OL_NUMBER is set to a unique 
		  value within all the ORDER-LINE rows that have the same OL_O_ID value, and 
		  OL_DIST_INFO is set to the content of S_DIST_xx, where xx represents the 
		  district number (OL_D_ID) */
		lped.ol_o_id=ped.o_id;
		lped.ol_d_id=zon.d_id;
		lped.ol_w_id=alm.w_id;
		lped.ol_number=i;
		lped.ol_i_id=pro.i_id;
		lped.ol_supply_w_id=ol_supply_w_id[i];
		lped.ol_delivery_d=0;
		memcpy(lped.ol_dist_info,exi.s_dist_01+(zon.d_id*25),25);
		DEBUGF("[nuevo_pedido] LineaPedido - [o:%u d:%u w:%u num:%u] i:%u sw:%u dd:%u qu:%u am:%u di:\"%s\"\n",
			lped.ol_o_id,lped.ol_d_id,lped.ol_w_id,lped.ol_number,lped.ol_i_id,lped.ol_supply_w_id,
			lped.ol_delivery_d,lped.ol_quantity,lped.ol_amount,lped.ol_dist_info);
		abm_insertar(bd->lineaspedido,&lped);

	}
	/* 2.6) The total-amount for the complete order is computed as: 
                sum(OL_AMOUNT) * (1 - C_DISCOUNT) * (1 + W_TAX + D_TAX) */
	DEBUGF("[nuevo_pedido] cantidad_total:%u c_discount:%u w_tax:%u d_tax:%u\n",
		cantidad_total,cli.c_discount,alm.w_tax,zon.d_tax);
	cantidad_total=(uint32_t)((float)cantidad_total*
				   (1-((float)cli.c_discount/10000))*
				   (1+(((float)alm.w_tax+(float)zon.d_tax)/10000)));
	DEBUGF("[nuevo_pedido] Cantidad total en centimos de euro: %u\n",cantidad_total);
}
void incrementa_d_next_o_id(RegZona *busqueda,RegZona *original) {
	original->d_next_o_id++;
}
void calcula_existencias(RegExistencias *busqueda,RegExistencias *original) {
	/* Cálculo de s_quantity */
	if (original->s_quantity>10) 
		original->s_quantity-=busqueda->s_quantity;
	else original->s_quantity=(original->s_quantity-busqueda->s_quantity)+91;
	
	original->s_ytd+=busqueda->s_quantity;          /* Cálculo de s_ytd */
	original->s_order_cnt++;                        /* Cálculo de s_order_cnt */
	original->s_remote_cnt+=busqueda->s_remote_cnt; /* Calculo de s_remote_cnt */
}

void incrementa_w_ytd(RegAlmacen *,RegAlmacen *);
void incrementa_d_ytd(RegZona *,RegZona *);
int  clientes_apellidos(RegCliente *,RegCliente *);
void actualiza_cliente(RegCliente *, RegCliente *);
void pago(BBDD *bd,FILE *fentrada) {
	int cliente_last,res,total;
	iterador it;
	RegCliente   cli,cliente_encontrado,grupo_clientes[20];
	RegAlmacen   alm;
	RegZona      zon;
	RegHistorico his;
	
	/* 1) Recuperar datos de entrada */
	if(fscanf(fentrada,RS_TERM_PAGO,&alm.w_id,&zon.d_id,&cliente_last,cli.c_last,&cli.c_d_id,&cli.c_w_id,
		&his.h_amount,&his.h_date)<1) {
		perror("");
		fprintf(stderr,"[pago] No puedo leer del fichero de entrada\n");
		return;
	}
	zon.d_w_id=alm.w_id;
	if(!cliente_last) cli.c_id=strtol(cli.c_last,NULL,10);
	
	DEBUGF("[pago] Generado en: w_id:%u d_id:%u - cantidad:%u fecha:%d\n",
		alm.w_id,zon.d_id,his.h_amount,his.h_date);
	#ifdef DEBUG
	if (cliente_last) DEBUGF("[pago] Cliente apellido:%s w_id:%u d_w:%u\n",cli.c_last,cli.c_w_id,cli.c_d_id);
	else DEBUGF("[pago] Cliente id:%u w_id:%u d_w:%u\n",cli.c_id,cli.c_w_id,cli.c_d_id);
	#endif

	/* 2) Transaccion */
	/* 2.1) The row in the WAREHOUSE table with matching W_ID is selected. W_NAME, W_STREET_1, W_STREET_2, 
                W_CITY, W_STATE, and W_ZIP are retrieved and W_YTD, the warehouse's year-to-date balance, is increased 
                by H_AMOUNT. */
	res=abm_buscar(bd->almacenes,&alm);
	if(res==ABM_404) {
		DEBUGF("[pago] ERROR Almacen no encontrado - w_id:%u\n",alm.w_id);
		return;
	}
	DEBUGF("[pago] Viejo w_ytd: %llu\n",alm.w_ytd);
	alm.w_ytd=his.h_amount;
	res=abm_modificar(bd->almacenes,&alm,(doFunc)&incrementa_w_ytd);
	if(res==ABM_404) {
		DEBUGF("[pago] ERROR Almacen no modificado - w_id:%u\n",alm.w_id);
		return;
	}
	DEBUGF("[pago] Nuevo w_ytd: %llu\n",alm.w_ytd);
	
	/* 2.2) The row in the DISTRICT table with matching D_W_ID and D_ID is selected. D_NAME, D_STREET_1, 
		D_STREET_2, D_CITY, D_STATE, and D_ZIP are retrieved and D_YTD, the district's year-to-date balance, is 
		increased by H_AMOUNT. */
	res=abm_buscar(bd->zonas,&zon);
	if(res==ABM_404) {
		DEBUGF("[pago] ERROR Zona no encontrada: w_id:%u d_id:%u\n",zon.d_w_id,zon.d_id);
		return;
	}
	DEBUGF("[pago] Viejo d_ytd: %llu\n",zon.d_ytd);
	zon.d_ytd=his.h_amount;
	res=abm_modificar(bd->zonas,&zon,(doFunc)&incrementa_d_ytd);
	if(res==ABM_404) {
		DEBUGF("[pago] ERROR Zona no modificada: w_id:%u d_id:%u\n",zon.d_w_id,zon.d_id);
		return;
	}
	DEBUGF("[pago] Nuevo d_ytd: %llu\n",zon.d_ytd);

	/* 2.2.1) Case 1, the customer is selected based on customer number: the row in the CUSTOMER table with matching 
		C_W_ID, C_D_ID and C_ID is selected. C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, C_CITY, 
		C_STATE, C_ZIP, C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, and C_BALANCE are 
		retrieved. C_BALANCE is decreased by H_AMOUNT. C_YTD_PAYMENT is increased by H_AMOUNT. 
		C_PAYMENT_CNT is incremented by 1. */
	if (!cliente_last) {
		DEBUGF("[pago] Cliente por identificador - c_id:%u c_w_id:%u c_d_id:%u\n",cli.c_id,cli.c_w_id,cli.c_d_id);
		res=abm_buscar(bd->clientes,&cli);
		if(res==ABM_404) {
			DEBUGS("[pago] ERROR Cliente no encontrado"); 
			return;
		}
	}
	
	/* 2.2.2) Case 2, the customer is selected based on customer last name: all rows in the CUSTOMER table with matching 
		C_W_ID, C_D_ID and C_LAST are selected sorted by C_FIRST in ascending order. Let n be the number of 
		rows selected. C_ID, C_FIRST, C_MIDDLE, C_STREET_1, C_STREET_2, C_CITY, C_STATE, C_ZIP, 
		C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, and C_BALANCE are retrieved from the 
		row at position (n/2 rounded up to the next integer) in the sorted set of selected rows from the CUSTOMER 
		table. C_BALANCE is decreased by H_AMOUNT. C_YTD_PAYMENT is increased by H_AMOUNT. 
		C_PAYMENT_CNT is incremented by 1. */
	else {
		DEBUGF("[pago] Cliente por apellido - c_d_id:%u c_w_id:%u c_last:\"%s\"\n",
			cli.c_d_id,cli.c_w_id,cli.c_last);
		
		abm_it_init(&it,bd->clientes,(itFunc)&clientes_apellidos,&cli);
		total=0;
		while( (res=abm_iterar(&it,&cliente_encontrado)) != ABM_FIN && total<20) {
			memcpy(grupo_clientes+total,&cliente_encontrado,sizeof(RegCliente));
			DEBUGF("[pago] %d Encontrado - c_id:%u c_d_id:%u c_w_id:%u c_last:\"%s\"\n",total,
				cliente_encontrado.c_id,cliente_encontrado.c_d_id,
				cliente_encontrado.c_w_id,cliente_encontrado.c_last);
			total++;
		}
		abm_it_fin(&it);
		
		if (total==0) {
			DEBUGS("[pago] ERROR ¿¡Ningun cliente encontrado por apellido, imposible!?");
			return;
		}
		memcpy(&cli,grupo_clientes+(total>>1),sizeof(RegCliente));	
		DEBUGF("[pago] Cliente seleccionado - c_id:%u c_w_id:%u c_d_id:%u\n",cli.c_id,cli.c_w_id,cli.c_d_id);
	}

	/* Tanto para 2.2.1 como para 2.2.2 - Mandamos modificar mas abajo*/
	DEBUGF("[pago] Balance cliente: %lld - Cantidad a pagar: %u\n",cli.c_balance,his.h_amount);
	cli.c_balance=his.h_amount;

	/* 2.3) If the value of C_CREDIT is equal to "BC", then C_DATA is also retrieved from the selected customer and the 
		following history information: C_ID, C_D_ID, C_W_ID, D_ID, W_ID, and H_AMOUNT, are inserted at the 
		left of the C_DATA field by shifting the existing content of C_DATA to the right by an equal number of bytes 
		and by discarding the bytes that are shifted out of the right side of the C_DATA field. The content of the 
		C_DATA field never exceeds 500 characters. The selected customer is updated with the new C_DATA field. If 
		C_DATA is implemented as two fields (see Clause 1.4.9), they must be treated and operated on as one single 
		field. */
	DEBUGF("[pago] Cliente datos:\"%s\"\n",cli.c_data);
	if (strncmp(cli.c_credit,"BC",2)==0) { 
		snprintf(cli.c_data,sizeof(cli.c_data),"%u %u %u %u %u %u",
			cli.c_id,cli.c_d_id,cli.c_w_id,zon.d_id,zon.d_w_id,his.h_amount);
		DEBUGF("[pago] Cliente añadir datos:\"%s\"\n",cli.c_data);
	}
	else cli.c_credit[0]='\0';

	/* Para 2.2.1, 2.2.2 y 2.3 */
	res=abm_modificar(bd->clientes,&cli,(doFunc)&actualiza_cliente);
	DEBUGF("[pago] Cliente modificado - balan:%lld  ytd:%llu cnt:%u data:\"%s\"\n",
		cli.c_balance,cli.c_ytd_payment,cli.c_payment_cnt,cli.c_data);
	
	/* 2.4) H_DATA is built by concatenating W_NAME and D_NAME separated by 4 spaces. */
	snprintf(his.h_data,sizeof(his.h_data),"%s    %s",alm.w_name,zon.d_name);
	
	/* 2.5) A new row is inserted into the HISTORY table with H_C_ID = C_ID, H_C_D_ID = C_D_ID, H_C_W_ID = 
		C_W_ID, H_D_ID = D_ID, and H_W_ID = W_ID. */
	his.h_c_id=cli.c_id;
	his.h_c_d_id=cli.c_d_id;
	his.h_c_w_id=cli.c_w_id;
	his.h_d_id=zon.d_id;
	his.h_w_id=alm.w_id;
	DEBUGF("[pago] Historico - c_id:%u cw_id:%u cd_id:%u d_id:%u w_id:%u\n",his.h_c_id,his.h_c_d_id,
		his.h_c_w_id,his.h_d_id,his.h_w_id);
	DEBUGF("[pago] Historico - fecha:%u cantidad:%u datos:\"%s\"\n",his.h_date,his.h_amount,his.h_data);
	le_insertar_final(bd->historico,&his);
}
void incrementa_w_ytd(RegAlmacen *busqueda,RegAlmacen *original) {
	original->w_ytd+=busqueda->w_ytd;
}
void incrementa_d_ytd(RegZona *busqueda, RegZona *original) {
	original->d_ytd+=busqueda->d_ytd;
}
int clientes_apellidos(RegCliente *busqueda,RegCliente *extra) {
	if (busqueda->c_w_id == extra->c_w_id &&
	    busqueda->c_d_id == extra->c_d_id &&    
	    strcmp(busqueda->c_last,extra->c_last)==0) return 1;
	return 0;
}
void actualiza_cliente(RegCliente *busqueda, RegCliente *original) {
	char tmp[501];
	original->c_balance-=busqueda->c_balance;
	original->c_ytd_payment+=busqueda->c_balance;
	original->c_payment_cnt++;
	if (original->c_data[0]!='\0') {
		snprintf(tmp,501,"%s %s",busqueda->c_data,original->c_data);
		memcpy(original->c_data,tmp,501);
	}
}

int pedidos_cliente(RegPedido *,RegPedido *);
void estado_pedido(BBDD *bd,FILE *fentrada) {
	int cliente_last,res,total,i,penc;
	RegAlmacen alm;
	RegZona    zon;
	RegCliente cli,grupo_clientes[20],cliente_encontrado;
	RegPedido  ped,pedido_encontrado;
	RegLineaPedido lped;
	iterador it;

	/* 1) Recupera datos de entrada */
	if(fscanf(fentrada,RS_TERM_ESTADOPEDIDO,&alm.w_id,&zon.d_id,&cliente_last,cli.c_last)<1) {
		perror("");
		fprintf(stderr,"[estado_pedido] No puedo leer del fichero de entrada\n");
		return;
	}
	zon.d_w_id=alm.w_id;
	if(!cliente_last) cli.c_id=strtol(cli.c_last,NULL,10);

	DEBUGF("[estado_pedido] Generado en: w_id:%u d_id:%u\n",alm.w_id,zon.d_id);
	#ifdef DEBUG
	if (cliente_last) DEBUGF("[estado_pedido] Cliente apellido:%s w_id:%u d_w:%u\n",cli.c_last,cli.c_w_id,cli.c_d_id);
	else DEBUGF("[estado_pedido] Cliente id:%u w_id:%u d_w:%u\n",cli.c_id,cli.c_w_id,cli.c_d_id);
	#endif
					

	/* 2) Transaccion */
	/* 2.1) Seleccionar al cliente */
	/* 2.2.1) Case 1, the customer is selected based on customer number: the row in the CUSTOMER table with matching 
		C_W_ID, C_D_ID, and C_ID is selected and C_BALANCE, C_FIRST, C_MIDDLE, and C_LAST are retrieved. */
	if (!cliente_last) {
		DEBUGF("[estado_pedido] Cliente por identificador - c_id:%u c_w_id:%u c_d_id:%u\n",
			cli.c_id,cli.c_w_id,cli.c_d_id);
		res=abm_buscar(bd->clientes,&cli);
		if(res==ABM_404) {
			DEBUGS("[estado_pedido] ERROR Cliente no encontrado"); 
			return;
		}
	}
	
	/* 2.2.2) Case 2, the customer is selected based on customer last name: all rows in the CUSTOMER table with matching 
		C_W_ID, C_D_ID and C_LAST are selected sorted by C_FIRST in ascending order. Let n be the number of 
		rows selected. C_BALANCE, C_FIRST, C_MIDDLE, and C_LAST are retrieved from the row at position n/2 
		rounded up in the sorted set of selected rows from the CUSTOMER table. */
	else {
		DEBUGF("[estado_pedido] Cliente por apellido - c_d_id:%u c_w_id:%u c_last:\"%s\"\n",
			cli.c_d_id,cli.c_w_id,cli.c_last);
		abm_it_init(&it,bd->clientes,(itFunc)&clientes_apellidos,&cli);
		total=0;
		while( abm_iterar(&it,&cliente_encontrado) != ABM_FIN && total<20) {
			memcpy(grupo_clientes+total,&cliente_encontrado,sizeof(RegCliente));
			DEBUGF("[estado_pedido] %d Encontrado - c_id:%u c_d_id:%u c_w_id:%u c_last:\"%s\"\n",total,
				cliente_encontrado.c_id,cliente_encontrado.c_d_id,
				cliente_encontrado.c_w_id,cliente_encontrado.c_last);
			total++;
		}
		abm_it_fin(&it);
		if (total==0) {
			DEBUGS("[estado_pedido] ERROR ¿¡Ningun cliente encontrado por apellido, imposible!?");
			return;
		}
		memcpy(&cli,grupo_clientes+(total>>1),sizeof(RegCliente));	
		DEBUGF("[estado_pedido] Cliente seleccionado - c_id:%u c_w_id:%u c_d_id:%u\n",
			cli.c_id,cli.c_w_id,cli.c_d_id);
	}	

	/* 2.3) The row in the ORDER table with matching O_W_ID (equals C_W_ID), O_D_ID (equals C_D_ID), O_C_ID 
		(equals C_ID), and with the largest existing O_ID, is selected. This is the most recent order placed by that 
		customer. O_ID, O_ENTRY_D, and O_CARRIER_ID are retrieved. */
	ped.o_w_id=cli.c_w_id;
	ped.o_d_id=cli.c_d_id;
	ped.o_c_id=cli.c_id;
	ped.o_id=0;
	
	/* Pedido Encontrado */
	penc=0;
	abm_it_init(&it,bd->pedidos,(itFunc)&pedidos_cliente,&ped);
	DEBUGF("[estado_pedido] Buscando pedidos del cliente id:%u w:%u d:%u\n",cli.c_id,cli.c_w_id,cli.c_d_id);
	while( abm_iterar(&it,&pedido_encontrado) != ABM_FIN ) {
		if (pedido_encontrado.o_id > ped.o_id || penc==0) {
			/* Ya tenemos un candidato */
			memcpy(&ped,&pedido_encontrado,sizeof(RegPedido));
			penc=1;
		}
	}
	abm_it_fin(&it);

	if (penc==0) {
		DEBUGS("[estado_pedido] ERROR No hay ningun pedido para ese cliente");
		return;
	}
	DEBUGF("[estado_pedido] Ultimo pedido encontrado: id:%u w:%u d:%u\n",ped.o_id,ped.o_w_id,ped.o_d_id);
	
	/* 2.4) All rows in the ORDER-LINE table with matching OL_W_ID (equals O_W_ID), OL_D_ID (equals O_D_ID), 
		and OL_O_ID (equals O_ID) are selected and the corresponding sets of OL_I_ID, OL_SUPPLY_W_ID, 
		OL_QUANTITY, OL_AMOUNT, and OL_DELIVERY_D are retrieved. */
	DEBUGF("[estado_pedido] Lineas de pedido: %u\n",ped.o_ol_cnt);
	lped.ol_o_id=ped.o_id;
	lped.ol_w_id=ped.o_w_id;
	lped.ol_d_id=ped.o_d_id;
	for(i=0;i<ped.o_ol_cnt;i++) {
		lped.ol_number=i;
		res=abm_buscar(bd->lineaspedido,&lped);
		if(res==ABM_404) {
			DEBUGF("[estado_pedido] ERROR Linea de pedido %u no encontrada\n",lped.ol_number);
			return;
		}
		DEBUGF("[estado_pedido] Linea %u (o:%u w:%u d:%u)  - item:%u cantidad:%u precio:%u fecha:%d\n",
			lped.ol_number,lped.ol_o_id,lped.ol_w_id,lped.ol_d_id,
			lped.ol_i_id,lped.ol_quantity,lped.ol_amount,lped.ol_delivery_d);
	}
}
int pedidos_cliente(RegPedido *original,RegPedido *datos) {
	if (original->o_w_id == datos->o_w_id &&
	    original->o_d_id == datos->o_d_id &&
	    original->o_c_id == datos->o_c_id) return 1;
	return 0;
}

int nuevospedidos_zona(RegNuevoPedido *,RegNuevoPedido *);
void cliente_envio(RegCliente *,RegCliente *);
void envio(BBDD *bd,FILE *fentrada) {
	RegAlmacen     alm;
	RegNuevoPedido nped,np_encontrado;
	RegPedido      ped;
	RegLineaPedido lped;
	RegCliente     cli;
	uint32_t  o_carrier_id;
	time_t     ol_delivery_d;
	int i,j,res;
	int64_t precio_total;
	iterador it;
	
	/* 1) Recuperar datos de entrada */
	if(fscanf(fentrada,RS_TERM_ENVIO,&alm.w_id,&o_carrier_id,&ol_delivery_d)<1) {
		perror("");
		fprintf(stderr,"[estado_pedido] No puedo leer del fichero de entrada\n");
		return;
	}
	DEBUGF("[envio] w_id:%u o_carrier_id:%u ol_delivery_d:%u\n",alm.w_id,o_carrier_id,ol_delivery_d);

	/* 2) Transaccion */
	nped.no_w_id=alm.w_id;
	for(i=0;i<CARD_ZONA;i++) {
		DEBUGF("[envio] Almacen %u Zona %u de %u\n",alm.w_id,i,CARD_ZONA-1);
	/* 2.1) The row in the NEW-ORDER table with matching NO_W_ID (equals W_ID) and NO_D_ID (equals D_ID) and 
		with the lowest NO_O_ID value is selected. This is the oldest undelivered order of that district. 
		NO_O_ID, the order number, is retrieved. If no matching row is found, then the delivery of an order for 
		this district is skipped. The condition in which no outstanding order is present at a given district
		must be handled by skipping the delivery of an order for that district only and resuming the delivery 
		of an order from all remaining districts of the selected warehouse. If this condition occurs in more 
		than 1%, or in more than one, whichever is greater, of the business transactions, it must be reported */
		nped.no_d_id=i;
		nped.no_o_id=UINT32_MAX;
		abm_it_init(&it,bd->nuevospedidos,(itFunc)&nuevospedidos_zona,&nped);
		while ( abm_iterar(&it,&np_encontrado) != ABM_FIN) {
			/* Demasiada informacion
			   DEBUGF("[envio] Nuevo Pedido encontrado o_id:%u\n",np_encontrado.no_o_id); */
			if (np_encontrado.no_o_id < nped.no_o_id) 
				memcpy(&nped,&np_encontrado,sizeof(RegNuevoPedido));
		}
		abm_it_fin(&it);
		if (nped.no_o_id == UINT32_MAX) {
			DEBUGF("[envio] No se encontro un pedido nuevo para w:%u y d:%u\n",nped.no_w_id,nped.no_d_id);
			continue;
		}
		DEBUGF("[envio] Nuevo Pedido Seleccionado: w:%u d:%u o_id:%u\n",nped.no_w_id,nped.no_d_id,nped.no_o_id);
	/* 2.2) The selected row in the NEW-ORDER table is deleted. */
		abm_borrar(bd->nuevospedidos,&nped);
 
	/* 2.3) The row in the ORDER table with matching O_W_ID (equals W_ ID), O_D_ID (equals D_ID), and O_ID 
		(equals NO_O_ID) is selected, O_C_ID, the customer number, is retrieved, and O_CARRIER_ID is updated. */
		ped.o_id = nped.no_o_id;
		ped.o_w_id=nped.no_w_id;
		ped.o_d_id=nped.no_d_id;;
		res=abm_buscar(bd->pedidos,&ped);
		if(res==ABM_404) {
			DEBUGF("[envio] ERROR El pedido id:%u w:%u d:%u no existe\n",ped.o_id,ped.o_w_id,ped.o_d_id);
			return;
		}
		DEBUGF("[envio] Pedido - id:%u w:%u d:%u - o_c_id:%u o_carrier_id:%d\n",
			ped.o_id,ped.o_w_id,ped.o_d_id,ped.o_c_id,ped.o_carrier_id);
		ped.o_carrier_id=o_carrier_id;
		abm_modificar(bd->pedidos,&ped,NULL);
		DEBUGF("[envio] Pedido (nuevo carrier) - id:%u w:%u d:%u - o_c_id:%u o_carrier_id:%d\n",
			ped.o_id,ped.o_w_id,ped.o_d_id,ped.o_c_id,ped.o_carrier_id);

	/* 2.4) All rows in the ORDER-LINE table with matching OL_W_ID (equals O_W_ID), OL_D_ID (equals O_D_ID), 
		and OL_O_ID (equals O_ID) are selected. All OL_DELIVERY_D, the delivery dates, are updated to the current 
		system time as returned by the operating system and the sum of all OL_AMOUNT is retrieved. */
		lped.ol_o_id=ped.o_id;
		lped.ol_w_id=ped.o_w_id;
		lped.ol_d_id=ped.o_d_id;
		precio_total=0;
		for(j=0;j<ped.o_ol_cnt;j++) {
			lped.ol_number=j;
			res=abm_buscar(bd->lineaspedido,&lped);
			if(res==ABM_404) {
				DEBUGF("[envio] ERROR Linea de pedido %u no encontrada\n",lped.ol_number);
				return;
			}
			DEBUGF("[envio] Linea %u (o:%u w:%u d:%u)  - item:%u cantidad:%u precio:%u fecha:%d\n",
				lped.ol_number,lped.ol_o_id,lped.ol_w_id,lped.ol_d_id,
				lped.ol_i_id,lped.ol_quantity,lped.ol_amount,lped.ol_delivery_d);
			/* Actualizar delivery_d */
			lped.ol_delivery_d=ol_delivery_d;
			abm_modificar(bd->lineaspedido,&lped,NULL);
			precio_total+=lped.ol_amount;
		}
		
	/* 2.5) The row in the CUSTOMER table with matching C_W_ID (equals W_ID), C_D_ID (equals D_ID), and C_ID 
		(equals O_C_ID) is selected and C_BALANCE is increased by the sum of all order-line amounts 
		(OL_AMOUNT) previously retrieved. C_DELIVERY_CNT is incremented by 1. */
		cli.c_w_id=alm.w_id;
		cli.c_d_id=i;
		cli.c_id=ped.o_c_id;
		res=abm_buscar(bd->clientes,&cli);
		if(res==ABM_404) {
			DEBUGF("[envio] ERROR Cliente w:%u d:%u id:%u No encontrado\n",cli.c_w_id,cli.c_d_id,cli.c_id);
			return;
		}
		DEBUGF("[envio] Cliente w:%u d:%u id:%u - balance:%lld delivery:%u\n",
			cli.c_w_id,cli.c_d_id,cli.c_id,cli.c_balance,cli.c_delivery_cnt);
		cli.c_balance=precio_total;
		abm_modificar(bd->clientes,&cli,(doFunc)&cliente_envio);
		DEBUGF("[envio] Cliente (actualizado) w:%u d:%u id:%u - balance:%lld delivery:%u\n",
			cli.c_w_id,cli.c_d_id,cli.c_id,cli.c_balance,cli.c_delivery_cnt);
	}

}
int nuevospedidos_zona(RegNuevoPedido *original,RegNuevoPedido *datos) {
	if(original->no_d_id == datos->no_d_id &&
	   original->no_w_id == datos->no_w_id) return 1;
	return 0;
}
void cliente_envio(RegCliente *busqueda,RegCliente *original) {
	original->c_balance+=busqueda->c_balance;
	original->c_delivery_cnt++;
}

int lineaspedido_rango(RegLineaPedido *,RegLineaPedido *);
void nivel_existencias(BBDD *bd,FILE *fentrada) {
	RegAlmacen     alm;
	RegZona        zon;
	RegLineaPedido lped,lp_encontrada;
	RegExistencias exi;
	iterador it;
	int limite,res;
	uint32_t contador=0;
	
	/* 1) Recuperar datos de entrada */
	if(fscanf(fentrada,RS_TERM_NIVELEXISTENCIAS,&alm.w_id,&zon.d_id,&limite)<1) {
		perror("");
		fprintf(stderr,"[estado_pedido] No puedo leer del fichero de entrada\n");
		return;
	}
	zon.d_w_id=alm.w_id;
	DEBUGF("[nivel_existencias] w_id:%u d_id:%u limite:%u\n",alm.w_id,zon.d_id,limite);
	
	/* 2) Transaccion */
	/* 2.1) The row in the DISTRICT table with matching D_W_ID and D_ID is selected and D_NEXT_O_ID is retrieved. */
	res=abm_buscar(bd->zonas,&zon);
	if(res == ABM_404) {
		DEBUGF("[nivel_existencias] ERROR Zona w:%u d:%u\n",zon.d_w_id,zon.d_id);
		return;
	}
	
	/* 2.2) All rows in the ORDER-LINE table with matching OL_W_ID (equals W_ID), OL_D_ID (equals D_ID), and 
		OL_O_ID (lower than D_NEXT_O_ID and greater than or equal to D_NEXT_O_ID minus 20) are selected. 
		They are the items for 20 recent orders of the district. */
	lped.ol_w_id=alm.w_id;
	lped.ol_d_id=zon.d_id;
	lped.ol_o_id=zon.d_next_o_id;
	exi.s_w_id=alm.w_id;
	DEBUGF("[nivel_existencias] Lineas pedido: w:%u d:%u o:[%u-%u]\n",
		lped.ol_w_id,lped.ol_d_id,lped.ol_o_id-20,lped.ol_o_id-1);
	abm_it_init(&it,bd->lineaspedido,(itFunc)lineaspedido_rango,&lped);
	while ( abm_iterar(&it,&lp_encontrada) != ABM_FIN ) {
		DEBUGF("[nivel_existencias] Linea encontrada: w:%u d:%u o:%u\n",
			lp_encontrada.ol_w_id,lp_encontrada.ol_d_id,lp_encontrada.ol_o_id);
	/* 2.3) All rows in the STOCK table with matching S_I_ID (equals OL_I_ID) and S_W_ID (equals W_ID) from the list 
		of distinct item numbers and with S_QUANTITY lower than threshold are counted (giving low_stock). 
 		Comment: Stocks must be counted only for distinct items. Thus, items that have been ordered more than once 
		in the 20 selected orders must be aggregated into a single summary count for that item. */
		exi.s_i_id=lp_encontrada.ol_i_id;
		res=abm_buscar(bd->existencias,&exi);
		if(res==ABM_404) {
			DEBUGF("[nivel_existencias] Existencia w:%u i:%u\n",exi.s_w_id,exi.s_i_id);
			continue;
		}
		/* Por hacer algo, ya que normalmente tendria que informarse por pantalla */
		if(exi.s_quantity < limite)  {
			DEBUGF("[nivel_existencias] Baja Cantadidad - actual:%u limite:%u\n",exi.s_quantity,limite);
			contador++;
		}
	}
	abm_it_fin(&it);
		
}
int lineaspedido_rango(RegLineaPedido *original,RegLineaPedido *datos) {
	if(original->ol_w_id == datos->ol_w_id &&
	   original->ol_d_id == datos->ol_d_id &&
	   original->ol_o_id <  datos->ol_o_id   &&
	   original->ol_o_id >= (datos->ol_o_id-20)) return 1;
	return 0;
}
