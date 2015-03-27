/** \defgroup ListaEnlazada Lista Simplemente enlazada. 
    @{
*/

/** \file listaenlazada.h 
 * Interfaz de la Lista Simplemente Enlazada.
 */

#ifndef __LISTAENLAZADA_H__
#define __LISTAENLAZADA_H__


/* Parmacs */
MAIN_ENV();

/* Constantes comunes */
#define OP_OK 0 ///< Resultado de la operacion correcto
#define OP_NODATA 1 ///< Resultado de la operacion sin datos

/** Estructura para manejar los enlaces al nodo siguiente. 
   Esto es lo que se añadirá a los registros genéricos que se quieran insertar
   en la lista 
*/
struct struct_LENodo {
	/** Puntero al siguiente elemento de la lista. Con valor NULL cuando no
	   Hay más nodos en la lista */
	struct struct_LENodo *siguiente;
};

/**   Almacena la informacion necesaria para manejar la lista. */
struct struct_ListaEnlazada {
	/** Puntero al primer nodo de la lista, o NULL cuando está vacía */
	struct struct_LENodo *principio;
	/** Puntero al ultimo nodo de la lista, o NULL cuando está vacía */
	struct struct_LENodo *fin;
	/** Tamaño del registro genérico usado */
	int regsize;
	/** Cerrojo de la lista */
	LOCKDEC(bloqueo);
};

/** Tipo de dato para los nodos de la lista enlazada */
typedef struct struct_LENodo LENodo;
/** Tipo de dato que representa a la lista enlazada. */
typedef struct struct_ListaEnlazada ListaEnlazada;

/* interfáz de uso de la lista enlazada */
ListaEnlazada *le_nueva(int);
void le_insertar_final(ListaEnlazada *,void *);
int  le_borrar_primero(ListaEnlazada *, void *); 
void le_destruir(ListaEnlazada *);

#endif


/** @} */
