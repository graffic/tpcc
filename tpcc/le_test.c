#include <stdio.h>
#include "listaenlazada.h"

#define MAX 300

int main(void) {
	int i,tmp;
	ListaEnlazada *lista=le_nueva(sizeof(int));
	for(i=0;i<MAX;i++) le_insertar_final(lista,&i);
	for(i=0;i<MAX;i++) {
		le_borrar_primero(lista,&tmp);
		if(i!=tmp) puts("ERROR");
	}
	le_destruir(lista);

	return 0;
}
