#ifndef __CARGADOR_H__
#define __CARGADOR_H__

#include "arbolbmas.h"
#include "listaenlazada.h"

struct struct_BBDD {
	Arbol *productos;
	Arbol *almacenes;
	Arbol *existencias;
	Arbol *zonas;
	Arbol *clientes;
	ListaEnlazada *historico;
	Arbol *pedidos;
	Arbol *lineaspedido;
	Arbol *nuevospedidos;
	uint32_t numServ;
	uint64_t numTran;
};

typedef struct struct_BBDD BBDD;

void cargador(BBDD*,char*);
#endif
