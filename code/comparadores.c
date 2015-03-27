#include <string.h>
#include "registros.h"

/** Ayuda a la comparación - Números enteros solos 
    Necesita de a y b.
*/
#define I_COMPARA_NUM(atrib) if (a->atrib > b->atrib) return 1;\
	else if (a->atrib < b->atrib) return -1;
/** Ayuda a la comparación - Números enteros en otro if 
    Necesita de a,b y de un if anterior.
*/
#define EI_COMPARA_NUM(atrib) else I_COMPARA_NUM(atrib)
/** Ayuda a la comparación - Cadenas de caracteres 
    Necesita de a,b y strres.
*/
#define I_COMPARA_STR(atrib) strres=strcmp(a->atrib,b->atrib);\
	if (strres>0) return 1;\
	else if(strres<0) return -1;

int comparar_productos(RegProducto *a, RegProducto *b) {
	/* Atributo i_id */
	I_COMPARA_NUM(i_id)
	/* Son iguales */
	else return 0;
}

int comparar_almacenes(RegAlmacen *a, RegAlmacen *b) {
	/* Atributo w_id */
	I_COMPARA_NUM(w_id)
	/* Son iguales */
	else return 0;
}

int comparar_existencias(RegExistencias *a, RegExistencias *b) {
	/* Atributo s_i_id */
	I_COMPARA_NUM(s_i_id)
	/* Atributo s_w_id */
	EI_COMPARA_NUM(s_w_id)
	/* Son iguales */
	else return 0;
}

int comparar_zonas(RegZona *a, RegZona *b) {
	/* Atributo d_id */
	I_COMPARA_NUM(d_id)
	/* Atributo d_w_id */
	EI_COMPARA_NUM(d_w_id)
	/* Son iguales */
	else return 0;
}

int comparar_clientes(RegCliente *a, RegCliente *b) {
	/* Atributo c_id */
	I_COMPARA_NUM(c_id)
	/* Atributo c_d_id */
	EI_COMPARA_NUM(c_d_id)
	/* Atributo c_w_id */
	EI_COMPARA_NUM(c_w_id)
	/* Son iguales */
	return 0;
}

int comparar_historico(RegHistorico *a, RegHistorico *b) {
	int strres;
	/* Atributo h_c_id */
	I_COMPARA_NUM(h_c_id)
	/* Atributo h_c_d_id */
	EI_COMPARA_NUM(h_c_d_id)
	/* Atributo h_c_w_id */
	EI_COMPARA_NUM(h_c_w_id)
	/* Atributo h_d_id */
	EI_COMPARA_NUM(h_d_id)
	/* Atributo h_w_id */
	EI_COMPARA_NUM(h_w_id)
	/* Atributo h_date */
	EI_COMPARA_NUM(h_date)
	/* Atributo h_amount */
	EI_COMPARA_NUM(h_amount)
	/* Atributo h_data[25] */
	I_COMPARA_STR(h_data)
	/* Son iguales */
	return 0;
}

int comparar_pedidos(RegPedido *a, RegPedido *b) {
	/* Atributo o_id */
	I_COMPARA_NUM(o_id)
	/* Atributo o_d_id */
	EI_COMPARA_NUM(o_d_id)
	/* Atributo o_w_id */
	EI_COMPARA_NUM(o_d_id)
	/* Son iguales */
	return 0;
}

int comparar_lineaspedido(RegLineaPedido *a, RegLineaPedido *b) {
	/* Atributo ol_o_id */
	I_COMPARA_NUM(ol_o_id)
	/* Atributo ol_d_id */
	EI_COMPARA_NUM(ol_d_id)
	/* Atributo ol_w_id */
	EI_COMPARA_NUM(ol_w_id)
	/* Atributo ol_number */
	EI_COMPARA_NUM(ol_number)
	/* Son iguales */
	return 0;
}

int comparar_nuevospedidos(RegNuevoPedido *a, RegNuevoPedido *b) {
	/* Atributo no_o_id */
	I_COMPARA_NUM(no_o_id)
	/* Atributo no_d_id */
	EI_COMPARA_NUM(no_d_id)
	/* Atributo no_w_id */
	EI_COMPARA_NUM(no_w_id)
	/* Son iguales */
	return 0;
}

