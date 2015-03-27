/** \ingroup ListaEnlazada
 * Implementacion de la Lista Simplemente Enlazada.
 *
 * Con las siguientes funcionalidades:
 * - Insertar al final.
 * - Borrar (y extraer a la vez) del final.
 * 
 * La lista almacena registros genéricos pero todos del mismo tamaño
 *
 * La lista es identificada por una estructura de tipo ListaEnlazada, cuyo
 * puntero será pasado al resto de funciones para operar sobre ella.
 *
 * La organizacion interna de la lista se hace por nodos de tipo LENodo. La 
 * funcion de estos nodos es la de añadir al registro que se quiera almacenar
 * un puntero al siguiente elemento.
 *
 * Al destruir una lista, se destruyen todos los registros almacenados.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "listaenlazada.h"


/** Inicializa un identificador de lista enlazada.
    - Se inicializan los punteros a null
    - Se almacena el tamaño del registro.
    - Se inicializa el cerrojo.

    \param[in] regsize Tamaño del registro de datos genérico con el que trabajar.
    \return un puntero a la estructura de lista enlazada, que estará 
            inicializada como vacía.
*/
ListaEnlazada *le_nueva(int regsize) {
	ListaEnlazada *nuevalista;

	DEBUGF("[le_nueva](regsize: %d)\n",regsize);
	DEBUGF("[le_nueva] Reservando memoria para la lista: %d\n",sizeof(ListaEnlazada));
	nuevalista=(ListaEnlazada *)G_MALLOC(sizeof(ListaEnlazada));
	
	/* Poner valores iniciales */
	nuevalista->principio=NULL;
	nuevalista->fin=NULL;
	nuevalista->regsize=regsize;
	LOCKINIT(nuevalista->bloqueo);

	/* Devolver la lista */ 
	/** \retval nuevalista Puntero a la estructura de lista. */
	return nuevalista;
}

/** Inserta al final de la lista.
    - Reserva memoria para el registro y la estructura de nodo.
    - Actualiza los punteros para colocar el nodo.
    - Copia los datos del nodo.

    \param[in] lista Lista enlazada sobre la que operar
    \param[in] registro Puntero al registro que insertar en la lista.
*/
void le_insertar_final(ListaEnlazada *lista,void *registro) {
	LENodo *newrecord;

	DEBUGF("[le_insertar_final](lista: %p, registro: %p\n",lista,registro);
	
	newrecord=G_MALLOC(lista->regsize+sizeof(LENodo));
	DEBUGF("[le_insertar_final] Nuevo registro %p de %d bytes\n",newrecord,lista->regsize+sizeof(LENodo));
	/* Como insertamos al final, el siguiente siempre es NULL */
	newrecord->siguiente=NULL;
	/* Rellenar de datos */
	DEBUGF("[le_insertar_final] Copiando de %p a %p %d bytes\n",registro,newrecord+1,lista->regsize);
	memcpy(newrecord+1,registro,lista->regsize);

	/* Si principio es NULL, la lista está vacia, y el nodo que insertemos es el primero y el último */
	LOCK(lista->bloqueo)
	if(lista->principio==NULL) {
		DEBUGS("[le_insertar_final] La lista está vacía");
		lista->principio=newrecord;
		lista->fin=newrecord;
	}
	/* Si no, decimos al nodo final que el siguiente es el nodo nuevo */
	else {
		DEBUGF("[le_insertar_final] La lista no está vacia, añadiendo despues de %p\n",lista->fin);
		lista->fin->siguiente=newrecord;
		lista->fin=newrecord;
	}
	UNLOCK(lista->bloqueo)
}

/** Borra el primer elemento de la lista y lo devuelve.
    - Primero comprueba si la lista está vacía.
    - En caso de no estarlo, copia los datos al registro.
    - Y luego actualiza los punteros internos.

    \param[in]  lista    Lista enlazada sobre la que operar.
    \param[out] registro Lugar donde depostiar el registro borrado.
    \returns Un entero indicando el resultado de la operación.
*/
int le_borrar_primero(ListaEnlazada *lista, void *registro) {
	DEBUGF("[le_borrar_primero](lista: %p, registro:%p)\n",lista,registro);
	
	/* Si hay datos */
	LOCK(lista->bloqueo)
	if (lista->principio!=NULL) {
		LENodo *tmp;
		
		DEBUGF("[le_borrar_primero] lista->principio no es NULL, es %p\n",lista->principio);
		
		/* Actualizamos estructura */
		tmp=lista->principio;
		lista->principio=lista->principio->siguiente;
		DEBUGF("[le_borrar_primero] Nuevo principio: %p\n",lista->principio);
		if(lista->principio==NULL) {
			lista->fin==NULL;
			DEBUGS("[le_borrar_primero] lista->fin es ahora NULL");
		}
		UNLOCK(lista->bloqueo)
		
		/* Copiamos los datos */
		if (registro!=NULL) {
			memcpy(registro,tmp+1,lista->regsize);
			DEBUGF("[le_borrar_primero] Copiando %d bytes de %p a %p\n",
				lista->regsize,lista->principio+1,registro);
		}
		/* Liberamos memoria */
		G_FREE(tmp);
		/** \retval OP_OK Operacion Correcta */
		return OP_OK;
	}
	UNLOCK(lista->bloqueo)
	/** \retval OP_NODATA Sin datos debido a que la lista estaba vacía */
	return OP_NODATA;
}

/** Destruye una lista enlazada junto con los nodos restantes.
    Va borrando todos los nodos uno a uno.
 
    \param[in]  lista    Lista enlazada sobre la que operar.
 */
void le_destruir(ListaEnlazada *lista) {
	DEBUGF("[le_destruir](lista:%p)\n",lista);
	
	while(le_borrar_primero(lista,NULL)==OP_OK);
	DEBUGS("[le_destruir] La lista está vacía");
}

