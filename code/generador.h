#ifndef __GENERADOR_H__
#define __GENERADOR_H__

#define FICHERO_PRODUCTOS     "productos.txt"
#define FICHERO_ALMACENES     "almacenes.txt"
#define FICHERO_EXISTENCIAS   "existencias.txt"
#define FICHERO_ZONAS         "zonas.txt"
#define FICHERO_CLIENTES      "clientes.txt"
#define FICHERO_HISTORICO     "historico.txt"
#define FICHERO_PEDIDOS       "pedidos.txt"
#define FICHERO_LINEASPEDIDO  "lineaspedido.txt"
#define FICHERO_NUEVOSPEDIDOS "nuevospedidos.txt"
#define FICHERO_CONSTANTES    "constantes.txt"


#define CARD_ALMACEN      1
#define CARD_ZONA         10
#define CARD_CLIENTE      30000
#define CARD_PEDIDO       CARD_CLIENTE
#define CARD_NUEVOPEDIDO  CARD_CLIENTE*0.3
#define CARD_PRODUCTO     100000
#define CARD_EXISTENCIAS  CARD_PRODUCTO
#define CARD_TRANSACCION  100
#define CARD_SERVIDOR     1

#endif
