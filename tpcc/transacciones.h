#ifndef __TRANSACCIONES_H__
#define __TRANSACCIONES_H__

#include "cargador.h"

void nuevo_pedido     (BBDD*,FILE*);
void pago             (BBDD*,FILE*);
void estado_pedido    (BBDD*,FILE*);
void envio            (BBDD*,FILE*);
void nivel_existencias(BBDD*,FILE*);

#endif
