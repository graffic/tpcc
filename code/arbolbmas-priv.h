#ifndef __ARBOLBMAS_PRIV_H__
#define __ARBOLBMAS_PRIV_H__

/* Atributos de un nodo */
/** Mascara que filtra todos los atributos de un nodo */
#define NODE_ATRIB       0xf
/** Atributo de nodo interno (bit 1) */
#define NODE_ATRIB_INNER 1
/** Atributo de nodo hoja (bit 2) */
#define NODE_ATRIB_LEAF  2

/* Codigos de las inserciones */
#define INS_INSERTED 0
#define INS_NOT_INSERTED 1

/* Macros */

/* Operaciones con el arbol */
#define getRoot(arb)         arb->raiz
#define setRoot(arb,nodo)    arb->raiz=nodo,nodo->link=NULL
#define getRegSize(arb)      arb->regsize
#define getCmpFunc(arb)      arb->comparar
#define arbolVacio(arb)      (arb==NULL || arb->raiz==NULL || arb->raiz->ocupados==0)
#define compareKeys(arb,a,b) arb->comparar(a,b)
#define getFirstNode(arb)    arb->listaHojas
#define setFirstNode(ar,no)  ar->listaHojas=no

/* Preguntas */
#define nodo_lleno(nodo) (nodo->ocupados==NODE_SIZE)
#define nodo_vacio(nodo) (nodo->ocupados==0)
#define nodo_pocos(nodo) (nodo->ocupados<NODE_SIZE_MIN)
#define nodo_hoja(nodo)	   ((nodo)->atributos & NODE_ATRIB_LEAF)
#define nodo_interno(nodo) ((nodo)->atributos & NODE_ATRIB_INNER)

/* NodeSize nodo->ocupados */
#define getNodeSize(nodo)      (nodo->ocupados)
#define setNodeSize(nodo,tama) (nodo->ocupados=tama)

/* Key */
#define getKeySize(nodo)           nodo->keysize
#define getNodeKey(nodo,pos)       ((void *)(nodo+1))+((pos)*getKeySize(nodo))
#define setNodeKey(nodo,pos,clave) memcpy(getNodeKey(nodo,pos),clave,getKeySize(nodo))
#define getFirstKey(nodo)          getNodeKey(nodo,0)
#define getLastKey(nodo)           getNodeKey(nodo,getNodeSize(nodo)-1)

/* Busquedas rápidas y sencillas */
#define searchNodeLeaf(origen,destino,anterior,registro) \
		destino=getRoot(arb);          \
		anterior=NULL;                 \
        	while(nodo_interno(destino)) { \
			anterior=destino;      \
	                destino=getInnerLink(destino,getNextLinkPos(destino,registro,getCmpFunc(arb))); \
		}		


/* Cuando un nodo es interno, hay que tener en cuenta el enlace de más.
 * por lo que aunque su tamaño sea 0, siempre tiene un enlace de más:
 * - Nodo hoja: ocupados+minimo-1 <= maximo  Menos 1 ya que hemos bajado del minimo un punto
 * - Nodo interno: ocupados+minimo <= maximo Aqui sumamos uno del enlace que siempre queda */
#define nodeCanMerge(nodo) ((nodo->ocupados+NODE_SIZE_MIN-(nodo_hoja(nodo)?1:0))<= NODE_SIZE)

#define setInnerLink(nodo,posicion,enlace) (nodo->enlaces.interno[posicion]=enlace)
#define getInnerLink(nodo,posicion)        (nodo->enlaces.interno[posicion])
#define setLeafLink(nodo,posicion,enlace) (nodo->enlaces.externo[posicion]=enlace)
#define getLeafLink(nodo,posicion)        (nodo->enlaces.externo[posicion])

#define initSetLink(nodo) node_links=(nodo_interno(nodo)?(void *)nodo->enlaces.interno:(void *)nodo->enlaces.externo)
#define setLink(posicion,enlace) node_links[posicion]=enlace
#define getLink(nodo,posicion) ( (nodo_interno(nodo)) ?  ((void *)getInnerLink(nodo,posicion)) : ((void *)getLeafLink(nodo,posicion)) )

/* Extra - Memoria */
#define memFree(zona) free(zona)

#endif

