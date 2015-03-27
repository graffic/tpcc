/**\ingroup tpcc
   \file registros.h
   Definicion de registros usados en las tablas del TPC-C.
   
   Un registro tiene siempre, como primeros campos, los campos que son clave 
   Para la definicion de atributos:
   - identificadores de cualquier tipo: uint32_t atrib;
   - texto tamaño variable, N: char atrib[N+1];
   - texto tamaño fijo, N: char atrib[N+1];
   - Numeros hasta 9 digitos: uint32_t atrib;
   - Numeros hasta 19 digitos: uint64_t atrib;
   - Fecha y hora: time_t atrib;
*/

#ifndef __REGISTROS_H__
#define __REGISTROS_H__

#include <sys/types.h>
#include <stdint.h>

/* 1) Registro Almacen */
/** Tamaño de la clave de los registro del tipo almacén */
#define REGALMACEN_KEYSIZE sizeof(struct {uint32_t w_id;})
/** Cadena para volcado del registro almacen via *printf */ 
#define DUMPSTRING_ALMACEN "%u\t%s\t%s\t%s\t%s\t%s\t%s\t%u\t%llu\n"
/** Cadena para la lectura del registro almacen via *scanf */
#define READSTRING_ALMACEN "%u\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%u\n%llu"
/** Parametros a usar con la cadena de volcado, partiendo de un registro almacen (alm) */
#define DUMPPARAM_ALMACEN(alm) alm.w_id, alm.w_name, alm.w_street_1, alm.w_street_2, alm.w_city,\
                               alm.w_state, alm.w_zip, alm.w_tax, alm.w_ytd
/** Parametros a usar con la cadena de lectura, partiendo de un registro almacen (alm) */
#define READPARAM_ALMACEN(alm) &alm.w_id, alm.w_name, alm.w_street_1, alm.w_street_2, alm.w_city,\
                               alm.w_state, alm.w_zip, &alm.w_tax, &alm.w_ytd
/** Registro equivalente a una tupla de la tabla Almacen */
struct struct_RegAlmacen {
	/* Campos Clave */
	uint32_t w_id;           ///< 2*W unique IDs W Warehouses are populated
	/* Resto Campos */
	char      w_name[11];     ///< variable text, size 10
	char      w_street_1[21]; ///< variable text, size 20
	char      w_street_2[21]; ///< variable text, size 20
	char      w_city[21];     ///< variable text, size 20
	char      w_state[3];     ///< fixed text, size 2
	char      w_zip[10];      ///< fixed text, size 9 
	uint32_t w_tax;          ///< numeric, 4 digits Sales tax  
	uint64_t w_ytd;          ///< numeric, 12 digits Year to date balance
};

/* 2) Registro Zona */
/** Tamaño de la clave de los registro del tipo zona */
#define REGZONA_KEYSIZE sizeof(struct {uint32_t d_id;uint32_t d_w_id;})
/** Cadena para volcado del registro zona via *printf */
#define DUMPSTRING_ZONA "%u\t%u\t%s\t%s\t%s\t%s\t%s\t%s\t%u\t%llu\t%u\n"
/** Cadena para la lectura del registro zona via *scanf */
#define READSTRING_ZONA "%u\t%u\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%u\t%llu\t%u\n"
/** Parametros a usar con la cadena de volcado, partiendo de un registro zona (zon) */
#define DUMPPARAM_ZONA(zon) zon.d_id, zon.d_w_id, zon.d_name, zon.d_street_1, zon.d_street_2, zon.d_city,\
                            zon.d_state, zon.d_zip, zon.d_tax, zon.d_ytd, zon.d_next_o_id 
/** Parametros a usar con la cadena de lectura, partiendo de un registro zona (zon) */
#define READPARAM_ZONA(zon) &zon.d_id, &zon.d_w_id, zon.d_name, zon.d_street_1, zon.d_street_2, zon.d_city,\
                            zon.d_state, zon.d_zip, &zon.d_tax, &zon.d_ytd, &zon.d_next_o_id
/** Registro equivalente a una tupla de la tabla Zona */
struct struct_RegZona {
	/* Campos clave */
	uint32_t d_id;   ///< 20 unique IDs 10 are populated per warehouse 
	uint32_t d_w_id; ///< 2*W unique IDs
	/* Otros Campos */
	char      d_name[11];     ///< variable text, size 10  
	char      d_street_1[21]; ///< variable text, size 20  
	char      d_street_2[21]; ///< variable text, size 20  
	char      d_city[21];     ///< variable text, size 20  
	char      d_state[3];     ///< fixed text, size 2  
	char      d_zip[9];       ///< fixed text, size 9  
	uint32_t d_tax;          ///< numeric, 4 digits Sales tax 
	uint64_t d_ytd;          ///< numeric, 12 digits Year to date balance 
	uint32_t d_next_o_id;    ///< 10,000,000 unique IDs Next available Order number 
};

/* 3) Registro Cliente */
/** Tamaño de la clave de los registro del tipo cliente */
#define REGCLIENTE_KEYSIZE sizeof(struct {uint32_t c_id;uint32_t c_d_id;uint32_t c_w_id;})
/** Cadena para volcado del registro cliente via *printf */
#define DUMPSTRING_CLIENTE "%u\t%u\t%u\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%lu\t%s\t%llu\t%u\t%lld\t%llu\t%u\t%u\t%s\n"
/** Cadena para la lectura del registro cliente via *scanf */
#define READSTRING_CLIENTE "%u\t%u\t%u\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%lu\t%[^\t]\t%llu\t%u\t%lld\t%llu\t%u\t%u\t%[^\n]\n"
/** Parametros a usar con la cadena de volcado, partiendo de un registro cliente (cl) */
#define DUMPPARAM_CLIENTE(cl) cl.c_id, cl.c_d_id, cl.c_w_id, cl.c_first, cl.c_middle, cl.c_last, cl.c_street_1,\
                              cl.c_street_2, cl.c_city, cl.c_state, cl.c_zip, cl.c_phone, cl.c_since, cl.c_credit,\
                              cl.c_credit_lim, cl.c_discount, cl.c_balance, cl.c_ytd_payment, cl.c_payment_cnt,\
                              cl.c_delivery_cnt, cl.c_data
/** Parametros a usar con la cadena de lectura, partiendo de un registro cliente (cl) */
#define READPARAM_CLIENTE(cl) &cl.c_id, &cl.c_d_id, &cl.c_w_id, cl.c_first, cl.c_middle, cl.c_last, cl.c_street_1,\
                              cl.c_street_2, cl.c_city, cl.c_state, cl.c_zip, cl.c_phone, &cl.c_since, cl.c_credit,\
                              &cl.c_credit_lim, &cl.c_discount, &cl.c_balance, &cl.c_ytd_payment, &cl.c_payment_cnt,\
                              &cl.c_delivery_cnt, cl.c_data
/** Registro equivalente a una tupla de la tabla Cliente */
struct struct_RegCliente {
	/* Campos clave */
	uint32_t c_id;   ///< C_ID 96,000 unique IDs 3,000 are populated per district 
	uint32_t c_d_id; ///< C_D_ID 20 unique IDs  
	uint32_t c_w_id; ///< C_W_ID 2*W unique IDs
	/* Otros campos */
	char      c_first[17];    ///< C_FIRST variable text, size 16  
	char      c_middle[3];    ///< C_MIDDLE fixed text, size 2  
	char      c_last[17];     ///< C_LAST variable text, size 16  
	char      c_street_1[21]; ///< C_STREET_1 variable text, size 20  
	char      c_street_2[21]; ///< C_STREET_2 variable text, size 20  
	char      c_city[21];     ///< C_CITY variable text, size 20  
	char      c_state[3];     ///< C_STATE fixed text, size 2  
	char      c_zip[10];      ///< C_ZIP fixed text, size 9  
	char      c_phone [17];   ///< C_PHONE fixed text, size 16  
	time_t    c_since;        ///< C_SINCE date and time  
	char      c_credit[3];    ///< C_CREDIT fixed text, size 2 "GC"=good, "BC"=bad 
	uint64_t c_credit_lim;   ///< C_CREDIT_LIM numeric, 12 digits  
	uint32_t c_discount;     ///< C_DISCOUNT numeric, 4 digits  
	int64_t c_balance;        ///< C_BALANCE signed numeric, 12 digits  
	uint64_t c_ytd_payment;  ///< C_YTD_PAYMENT numeric, 12 digits  
	uint32_t c_payment_cnt;  ///< C_PAYMENT_CNT numeric , 4 digits  
	uint32_t c_delivery_cnt; ///< C_DELIVERY_CNT numeric, 4 digits  
	char      c_data[501];    ///< C_DATA variable text, size 500 Miscellaneous information 
};

/* 4) Registro Histórico */
/** Tamaño de la clave de los registro del tipo histórico */
#define REGHISTORICO_KEYSIZE 0
#define DUMPSTRING_HISTORICO "%u\t%u\t%u\t%u\t%u\t%lu\t%u\t%s\n"
#define READSTRING_HISTORICO "%u\t%u\t%u\t%u\t%u\t%lu\t%u\t%[^\n]\n"
#define DUMPPARAM_HISTORICO(his) his.h_c_id, his.h_c_d_id, his.h_c_w_id, his.h_d_id, his.h_w_id, his.h_date,\
                                 his.h_amount, his.h_data
#define READPARAM_HISTORICO(his) &his.h_c_id, &his.h_c_d_id, &his.h_c_w_id, &his.h_d_id, &his.h_w_id, &his.h_date,\
                                 &his.h_amount, his.h_data
/** Registro equivalente a una tupla de la tabla Historico */
struct struct_RegHistorico {
	/* Campos clave */
	/* Otros campos */
	uint32_t h_c_id;     ///< H_C_ID 96,000 unique IDs  
	uint32_t h_c_d_id;   ///< H_C_D_ID 20 unique IDs  
	uint32_t h_c_w_id;   ///< H_C_W_ID 2*W unique IDs  
	uint32_t h_d_id;     ///< H_D_ID 20 unique IDs  
	uint32_t h_w_id;     ///< H_W_ID 2*W unique IDs  
	time_t    h_date;     ///< H_DATE date and time  
	uint32_t h_amount;   ///< H_AMOUNT numeric, 6 digits  
	char      h_data[25]; ///< H_DATA variable text, size 24 Miscellaneous information 
};

/* 5) Registro NuevoPedido */
/** Tamaño de la clave de los registro del tipo nuevo pedido */
#define REGNUEVOPEDIDO_KEYSIZE sizeof(struct {uint32_t no_o_id;uint32_t no_d_id;uint32_t no_w_id;})
#define DUMPSTRING_NUEVOPEDIDO "%u\t%u\t%u\n"
#define READSTRING_NUEVOPEDIDO DUMPSTRING_NUEVOPEDIDO
#define DUMPPARAM_NUEVOPEDIDO(np) np.no_o_id, np.no_d_id, np.no_w_id
#define READPARAM_NUEVOPEDIDO(np) &np.no_o_id, &np.no_d_id, &np.no_w_id
/** Registro equivalente a una tupla de la tabla NuevoPedido */
struct struct_RegNuevoPedido {
	/* Campos clave */
	uint32_t no_o_id; ///< NO_O_ID 10,000,000 unique IDs  
	uint32_t no_d_id; ///< NO_D_ID 20 unique IDs  
	uint32_t no_w_id; ///< NO_W_ID 2*W unique IDs  
	/* Otros campos */
};

/* 6) Registro Pedido */
/** Tamaño de la clave de los registro del tipo pedido */
#define REGPEDIDO_KEYSIZE sizeof(struct {uint32_t o_id;uint32_t o_d_id;uint32_t o_w_id;})
#define DUMPSTRING_PEDIDO "%u\t%u\t%u\t%u\t%lu\t%u\t%u\t%u\n"
#define READSTRING_PEDIDO DUMPSTRING_PEDIDO
#define DUMPPARAM_PEDIDO(ped) ped.o_id, ped.o_d_id, ped.o_w_id, ped.o_c_id, ped.o_entry_d,\
                              ped.o_carrier_id, ped.o_ol_cnt, ped.o_all_local
#define READPARAM_PEDIDO(ped) &ped.o_id, &ped.o_d_id, &ped.o_w_id, &ped.o_c_id, &ped.o_entry_d,\
                              &ped.o_carrier_id, &ped.o_ol_cnt, &ped.o_all_local
/** Registro equivalente a una tupla de la tabla Pedido */
struct struct_RegPedido {
	/* Campos clave */
	uint32_t o_id;   ///< O_ID 10,000,000 unique IDs  
	uint32_t o_d_id; ///< O_D_ID 20 unique IDs  
	uint32_t o_w_id; ///< O_W_ID 2*W unique IDs
	/* Otros campos */
	uint32_t o_c_id;       ///< O_C_ID 96,000 unique IDs  
	time_t    o_entry_d;    ///< O_ENTRY_D date and time  
	uint32_t o_carrier_id; ///< O_CARRIER_ID 10 unique IDs, or null  
	uint32_t o_ol_cnt;     ///< O_OL_CNT from 5 to 15 Count of Order-Lines 
	uint32_t o_all_local;  ///< O_ALL_LOCAL numeric, 1 digit  
};

/* 7) Registro LineaPedido */
/** Tamaño de la clave de los registro del tipo linea de pedido */
#define REGLINEAPEDIDO_KEYSIZE sizeof(struct {uint32_t ol_i_id;uint32_t ol_d_id;uint32_t ol_w_id;uint32_t ol_number;})
#define DUMPSTRING_LINEAPEDIDO "%u\t%u\t%u\t%u\t%u\t%u\t%lu\t%u\t%u\t%s\n"
#define READSTRING_LINEAPEDIDO "%u\t%u\t%u\t%u\t%u\t%u\t%lu\t%u\t%u\t%[^\n]\n"
#define DUMPPARAM_LINEAPEDIDO(lp) lp.ol_o_id, lp.ol_d_id, lp.ol_w_id, lp.ol_number, lp.ol_i_id,\
                                  lp.ol_supply_w_id, lp.ol_delivery_d, lp.ol_quantity,\
                                  lp.ol_amount, lp.ol_dist_info
#define READPARAM_LINEAPEDIDO(lp) &lp.ol_o_id, &lp.ol_d_id, &lp.ol_w_id, &lp.ol_number, &lp.ol_i_id,\
                                  &lp.ol_supply_w_id, &lp.ol_delivery_d, &lp.ol_quantity,\
                                  &lp.ol_amount, lp.ol_dist_info
/** Registro equivalente a una tupla de la tabla LineaPedido */
struct struct_RegLineaPedido {
	/* Campos clave */
	uint32_t ol_o_id;   ///< OL_O_ID 10,000,000 unique IDs  
	uint32_t ol_d_id;   ///< OL_D_ID 20 unique IDs  
	uint32_t ol_w_id;   ///< OL_W_ID 2*W unique IDs  
	uint32_t ol_number; ///< OL_NUMBER 15 unique IDs  
	/* Otros campos */
	uint32_t ol_i_id;          ///< OL_I_ID 200,000 unique IDs  
	uint32_t ol_supply_w_id;   ///< OL_SUPPLY_W_ID 2*W unique IDs  
	time_t    ol_delivery_d;    ///< OL_DELIVERY_D date and time, or null  
	uint32_t ol_quantity;      ///< OL_QUANTITY numeric, 2 digits  
	uint32_t ol_amount;        ///< OL_AMOUNT numeric, 6 digits  
	char      ol_dist_info[25]; ///< OL_DIST_INFO fixed text, size 24
};

/* 8) Registro Existencias */
/** Tamaño de la clave de los registros del tipo existencias */
#define REGEXISTENCIAS_KEYSIZE sizeof(struct {uint32_t s_id;uint32_t s_w_id;})
#define DUMPSTRING_EXISTENCIAS "%u\t%u\t%u\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%u\t%u\t%u\t%s\n"
#define READSTRING_EXISTENCIAS "%u\t%u\t%u\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%u\t%u\t%u\t%[^\n]\n"
#define DUMPPARAM_EXISTENCIAS(ex) ex.s_i_id, ex.s_w_id, ex.s_quantity, ex.s_dist_01, ex.s_dist_02, ex.s_dist_03,\
                                  ex.s_dist_04, ex.s_dist_05, ex.s_dist_06, ex.s_dist_07, ex.s_dist_08, ex.s_dist_09,\
                                  ex.s_dist_10, ex.s_ytd, ex.s_order_cnt, ex.s_remote_cnt, ex.s_data
#define READPARAM_EXISTENCIAS(ex) &ex.s_i_id, &ex.s_w_id, &ex.s_quantity, ex.s_dist_01, ex.s_dist_02, ex.s_dist_03,\
                                  ex.s_dist_04, ex.s_dist_05, ex.s_dist_06, ex.s_dist_07, ex.s_dist_08, ex.s_dist_09,\
                                  ex.s_dist_10, &ex.s_ytd, &ex.s_order_cnt, &ex.s_remote_cnt, ex.s_data
/** Registro equivalente a una tupla de la tabla Existencias */
struct struct_RegExistencias {
	/* Campos clave */
	uint32_t s_i_id;        ///< S_I_ID 200,000 unique IDs 100,000 populated per warehouse 
	uint32_t s_w_id;        ///< S_W_ID 2*W unique IDs  
	/* Otros campos */
	uint32_t s_quantity;    ///< S_QUANTITY numeric, 4 digits  
	char      s_dist_01[25]; ///< S_DIST_01 fixed text, size 24  
	char      s_dist_02[25]; ///< S_DIST_02 fixed text, size 24  
	char      s_dist_03[25]; ///< S_DIST_03 fixed text, size 24  
	char      s_dist_04[25]; ///< S_DIST_04 fixed text, size 24  
	char      s_dist_05[25]; ///< S_DIST_05 fixed text, size 24  
	char      s_dist_06[25]; ///< S_DIST_06 fixed text, size 24  
	char      s_dist_07[25]; ///< S_DIST_07 fixed text, size 24  
	char      s_dist_08[25]; ///< S_DIST_08 fixed text, size 24  
	char      s_dist_09[25]; ///< S_DIST_09 fixed text, size 24  
	char      s_dist_10[25]; ///< S_DIST_10 fixed text, size 24  
	uint32_t s_ytd;         ///< S_YTD numeric, 8 digits  
	uint32_t s_order_cnt;   ///< S_ORDER_CNT numeric, 4 digits  
	uint32_t s_remote_cnt;  ///< S_REMOTE_CNT numeric, 4 digits  
	char      s_data[51];    ///< S_DATA variable text, size 50 Make information 
};

/* 9) Registro Producto */

/** Tamaño de la clave de los registros del tipo producto */
#define REGPRODUCTO_KEYSIZE sizeof(struct {uint32_t i_id;})
#define DUMPSTRING_PRODUCTO "%u\t%u\t%s\t%u\t%s\n" 
#define READSTRING_PRODUCTO "%u\t%u\t%[^\t]\t%u\t%[^\n]\n"
#define DUMPPARAM_PRODUCTO(prod) prod.i_id, prod.i_im_id, prod.i_name, prod.i_price, prod.i_data
#define READPARAM_PRODUCTO(prod) &prod.i_id, &prod.i_im_id, prod.i_name, &prod.i_price, prod.i_data
/** Registro equivalente a una tupla de la tabla Productos */
struct struct_RegProducto {
	/* Campos clave */
	uint32_t i_id;       ///< I_ID 200,000 unique IDs 100,000 items are populated 
	/* Otros campos */
	uint32_t i_im_id;    ///< I_IM_ID 200,000 unique IDs Image ID associated to Item 
	char      i_name[25]; ///< I_NAME variable text, size 24  
	uint32_t i_price;    ///< I_PRICE numeric, 5 digits  
	char      i_data[51]; ///< I_DATA variable text, size 50 Brand  information 
};

/* 2) Tipos de datos */
typedef struct struct_RegAlmacen     RegAlmacen;    ///< Registro de la tabla de Almacen
typedef struct struct_RegZona        RegZona;       ///< Registro de la tabla de Zona
typedef struct struct_RegCliente     RegCliente;    ///< Registro de la tabla de Cliente
typedef struct struct_RegHistorico   RegHistorico;  ///< Registro de la tabla de Historico
typedef struct struct_RegNuevoPedido RegNuevoPedido;///< Registro de la tabla de Pedido
typedef struct struct_RegPedido      RegPedido;     ///< Registro de la tabla de NuevoPedido
typedef struct struct_RegLineaPedido RegLineaPedido;///< Registro de la tabla de LineaPedido
typedef struct struct_RegExistencias RegExistencias;///< Registro de la tabla de Existencias
typedef struct struct_RegProducto    RegProducto;   ///< Registro de la tabla de Producto

#endif

