/** \defgroup arbolbmas Arbol B+ 
  @{ */

/** Interfaz de uso para el arbol B+. */
#ifndef __ARBOLBMAS_H__
#define __ARBOLBMAS_H__

#include <stdint.h>
#include "readwrite.h"

/* 1) Datos fijos del arbol */

/** Capacidad del nodo, recordar que los nodos internos tienen un enlace más  */
#define NODE_SIZE 30

/** Minima cantidad de datos en un nodo.
 - Ayuda a ajustar la altura del nodo de manera mas o menos rápida
   segun el la cantidad mínima. mas mínimo, más temprano el ajuste.
 - Se debe de cumplir la regla: minimo <= (máximo)/2, para que el nodo pueda
   dividirse por la mitad cuando se llena y no incumpla el mínimo.
*/
#define NODE_SIZE_MIN 1

/** Maxima capacidad de un nodo.
    Debido a que es un unsigned short.
*/
#define NODE_MAX_CAP UINT16_MAX
/* Comprobacion de la regla del tamaño minimo */
#if NODE_SIZE_MIN > NODE_SIZE>>1
#error La regla de la cantidad minima es: minimo <= (máximo)/2
#endif

/* El limite minimo de un nodo debe de ser 1, asi cuando pasa a 0 saltan
   las "alarmas" */
#if NODE_SIZE_MIN == 0
#error El límite mas bajo  para un nodo es uno, no cero
#endif

/* Para todo esto, el tamaño minimo de un nodo de un arbol B+ es 3, debido
   a que si cuando esta lleno, hay que dividirlo y la clave de la mitad se
   usa como union entre los dos nodos: minimo 3 claves, una para la parte
   derecha, otra para la izquierda, y la del medio */
#if NODE_SIZE <3 
#error Ahora vas y me cuentas como divides un nodo interno en 2 trozos  y subes una clave al superior.
#endif

/* Valores devueltos */
#define ABM_OK    0 /**< Valor devuelto indicando que todo ha ido correctamente */
#define ABM_DUP   1 /**< Valor devuelto indicando que hay duplicados */
#define ABM_ERROR 2 /**< Valor devuelto indicando que ha ocurrido un error */
#define ABM_404   3 /**< Valor devuelto indicando que no se ha encontrado lo que se buscaba */
#define ABM_FIN   4 /**< Valor devuelto indicando que se ha llegado al final del arbol */

/* 2) Estructuras de datos */

/** Nodo del arbol: tanto interno como hoja */
struct struct_ABMNodo {
	/** Atributos del nodo */
        unsigned char  atributos;
	/** Entradas ocupadas en el nodo */
        unsigned short ocupados;
	/** Tamaño de las claves de este nodo.
	    Este campo es necesario para que el nodo sea autosuficiente y se
	    pueda operar con el sin necesidad de consultar la estructura
	    principal de arbol */
	unsigned int   keysize;
	/** Estado de sincronizacion para el nodo */
	rw_data        bloq;
        /** union es para diferenciar tipos.
         * - Si el nodo es interno, los enlaces de ese nodo son a otros nodos
         * - Si el nodo es hoja, los enlaces son a los datos
         */
        union {
		/** Punteros a nodos internos (En nodos internos) */
                struct struct_ABMNodo *interno[NODE_SIZE+1];
		/** Punteros a datos (En nodos hoja) */
                void *externo[NODE_SIZE];
        } enlaces;


        /** Enlace versatil a otro nodo.
         * - Si el nodo es interno, es un enlace al nodo padre
         * - Si el nodo es hoja, es un enlace al nodo siguiente */
        struct struct_ABMNodo *link;

	/* A partir de aquí irian las claves, pero al ser de tamaño variable */
};

/** Estructura que almacena informacion del arbol. */
struct struct_Arbol {
	/** Puntero al nodo raiz del arbol */
        struct struct_ABMNodo *raiz;
	/** Puntero a la hoja mas a la izquierda del arbol */
        struct struct_ABMNodo *listaHojas;
	/** Tamaño en bytes de la clave */
	unsigned int keysize;
	/** Tamaño en bytes del registro completo */
	unsigned int regsize;
	/** Funcion de comparación de 2 claves a y b.
	   Devuelve:
	   - -1 si a<b
	   -  0 si a=b
	   -  1 si a>b
	*/
	int (*comparar)(void *,void *);
	/** Estado de sincronización del arbol */
	rw_data      bloq;
};

/** \ingroup iterador
    Iterador para búsquedas secuenciales (sin usar la clave) */
struct struct_iterador {
	/** Nodo en el que nos encontramos */
	struct struct_ABMNodo *actual;
	/** Posicion dentro del nodo actual en la que nos encontramos */
	unsigned int           pos; 
	/** Tamaño de los registros a devolver */
	unsigned int           regsize;
	/** Funcion que dado un registro comprueba si cumple ciertas condiciones */
	int(*encaja)(void *,void *);
	/** Datos extra para la funcion encaja */
	void    *datos;
	/** Estructura de sincronizacion del arbol */
	rw_data *bloq;
};

/** Tipo de datos para los nodos del arbol */
typedef struct struct_ABMNodo ABMNodo;
/** Tipo de dato para el arbol */
typedef struct struct_Arbol   Arbol;
/** Tipo de dato que identifica a un puntero a funcion de comparación.
    Compara dos registros: a y b; de tal manera que:
    - Si a > b: devuelve 1.
    - Si a = b: devuelve 0.
    - Si a < b: devuelve -1 
*/
typedef int(*cmpFunc)(void *,void*);
/** Tipo de dato para el iterador */
typedef struct struct_iterador iterador; ///< \ingroup iterador
/** Tipo de dato para la funcion de busqueda del iterador 
    Dado un registro devuelve 0 o 1 si dicho registro encaja o no
    con las especificaciones indicadas en la funcion respectivamente.
    - Primer parametro de entrada: registro siguiente;
    - Segundo parametro de entrada: datos extra indicados al crear el iterador.
*/
typedef int(*itFunc)(void *,void *);            ///< \ingroup iterador
/** Tipo de dato para la funcion de modificacion de datos del arbol.
    Dados dos registros: uno el usado para la búsqueda y el que
    se ha encontrado en el arbol; realiza una operacion con el fin,
    de actualizar el registro encontrado usando los datos del registro
    dado para la busqueda. */
typedef void(*doFunc)(void *,void *);

/* 3) Declaración del interfáz */
/* General del arbol */
Arbol *abm_nuevo    (int,int,cmpFunc);
void   abm_destruir (Arbol *);
/* Operaciones */
int    abm_insertar (Arbol *,void *);
int    abm_buscar   (Arbol *,void *);
int    abm_iterar   (iterador *,void *);
int    abm_modificar(Arbol *,void *,doFunc);
int    abm_borrar   (Arbol *,void *);
/* Iterador para busqueda secuencial */
void  abm_it_init    (iterador*,Arbol *,itFunc,void*);
void  abm_it_fin     (iterador *);
/** @} */
#endif /* __ARBOLBMAS_H__ */

