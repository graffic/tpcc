#ifndef __COMPARADORES_H_
#define __COMPARADORES_H_

#include "registros.h"

int comparar_productos    (RegProducto *, RegProducto *);
int comparar_almacenes    (RegAlmacen *, RegAlmacen *);
int comparar_existencias  (RegExistencias *, RegExistencias *);
int comparar_zonas        (RegZona *, RegZona *);
int comparar_clientes     (RegCliente *, RegCliente *);
int comparar_historico    (RegHistorico *, RegHistorico *);
int comparar_pedidos      (RegPedido *, RegPedido *);
int comparar_lineaspedido (RegLineaPedido *, RegLineaPedido *);
int comparar_nuevospedidos(RegNuevoPedido *, RegNuevoPedido *);

#endif
