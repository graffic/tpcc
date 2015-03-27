
/** \ingroup arbolbmas */
/** Implementacion del arbol b+ */
#include <stdlib.h>
#include <string.h>
//#define DEBUG
//#define __PERFECT_DELETE_
#include "debug.h"
#include "arbolbmas.h"
#include "arbolbmas-priv.h"
#include "readwrite.h"

/** \defgroup privado Funciones Privadas */
/* Funciones privadas generales */
static ABMNodo *memNuevoNodo   (unsigned char,unsigned int); ///< \ingroup privado
static int      getNextLinkPos (ABMNodo *,void *,cmpFunc); 
static int      buscarClave    (cmpFunc,void *,ABMNodo *);
static ABMNodo *splitNodeLeaf  (Arbol *,ABMNodo *,ABMNodo *);
static int      insertNodeLeaf (Arbol *,ABMNodo *,void *);  
static void     insertNodeInner(Arbol *,ABMNodo *,ABMNodo *,ABMNodo *,char *);
static void     splitNodeInner (Arbol*,ABMNodo*,ABMNodo**,ABMNodo**,char*);   
static ABMNodo *deleteDescend  (Arbol*,void*,ABMNodo*,ABMNodo*,ABMNodo*,ABMNodo*,ABMNodo*,ABMNodo*,int*);
static void     removePos      (ABMNodo*,int);
static ABMNodo *nodeBalance    (ABMNodo*,ABMNodo*,ABMNodo*,cmpFunc);
static ABMNodo *nodeMerge      (ABMNodo*,ABMNodo*,ABMNodo*,cmpFunc);

/** Crea un arbol b+ nuevo.
  - Reserva la memoria necesaria para la estructura que identifica al arbol.
  - La inicializa a un estado que indique que el arbol está vacío.
  
  \param[in] regsize  Tamaño en bytes del registro a usar.
  \param[in] keysize  Tamaño de la clave del registro.
  \param[in] comparar Funcion que compara dos claves de dos registros.
  	     De tal manera que si al 1er parametro le llamamos A y al segundo B
	     , al llamar a la funcion comparar(A,B), esta devolverá:
	     - -1 si A<B
	     -  0 si A=B
	     -  1 si A>B

  \returns Un puntero al nuevo arbol creado.
*/
Arbol *abm_nuevo(int regsize,int keysize,cmpFunc comparar) {
	Arbol *nuevoArbol;
	ABMNodo *nodoInicial;

	DEBUGF("[abm_nuevo](regsize: %d, keysize: %d, comparar: %p)\n",regsize,keysize,comparar);

	/* Reservar memoria */
	DEBUGF("[abm_nuevo] Reservando %d bytes\n",sizeof(Arbol));
	nuevoArbol =(Arbol *)malloc(sizeof(Arbol));;
	nodoInicial=memNuevoNodo(NODE_ATRIB_LEAF,keysize);

	/* Rellenar la estructura */
	nuevoArbol->raiz=nodoInicial;
	nuevoArbol->listaHojas=nodoInicial;
	nuevoArbol->regsize=regsize;
	nuevoArbol->keysize=keysize;
	nuevoArbol->comparar=comparar;

	/* Inicializar la sincronizacion del arbol */
	DEBUGS("[abm_nuevo] Inicializando sincronizacion");
	rw_init(&nuevoArbol->bloq);
	
	return nuevoArbol;
}

/** Reserva memoria para un nuevo nodo y lo inicializa 

    \param[in] atributos Atributos con los que inicializar al nodo.
    \param[in] keysize Tamaño de cada clave que alberga el nodo
    \returns Un puntero al nuevo nodo creado.
*/
static ABMNodo *memNuevoNodo(unsigned char atributos,unsigned int keysize) {
        ABMNodo *nuevoNodo;
        
	DEBUGF("[memNuevoNodo](atributos: %hhu, keysize: %u)\n",atributos,keysize);
	
        /* Comprobar la asignacion correcta de memoria, si no salir inmediatamente 
	   Se asigna memoria tanto para la estructura de nodo: sizeof(ABMNodo), 
	   como para las claves: NODE_SIZE*keysize 
	*/
	// DEBUGF("[memNuevoNodo] Reservando %d bytes (%d + %d*%d)\n",sizeof(ABMNodo) + NODE_SIZE*keysize,sizeof(ABMNodo),NODE_SIZE,keysize);
	nuevoNodo=(ABMNodo *)malloc(sizeof(ABMNodo) + NODE_SIZE*keysize);
        if( !nuevoNodo ) {
                perror("");
		puts("No puedo asignar mas memoria");
		fflush(NULL);
                exit(1);
        }
        
        /* Valores por defecto de la estructura nodo */
        nuevoNodo->atributos=atributos;
        nuevoNodo->link=NULL;
	nuevoNodo->keysize=keysize;
	/* Usemos las macros que para algo están */
        setNodeSize(nuevoNodo,0);

	/* Inicializar la sincronizacion del nodo - Solo si es un nodo hoja (por ahora) */
	if (nodo_hoja(nuevoNodo)) rw_init(&nuevoNodo->bloq);

	return nuevoNodo;
}

/** Destruye un arbol.
    Borra un arbol, lo que incluye:
    - Los registros asociados.
    - Los nodos que lo componen.
    - La estructura que identifica al arbol.

    \param[in] arb Puntero al arbol a destruir.
*/    
void abm_destruir(Arbol *arb) {
	ABMNodo *act=getRoot(arb),*ant=NULL;
	int i,tmp;
	
	DEBUGF("[abm_destruir](arb: %p)\n",arb);
	/* Sincronizacion - Aunque no debiera ser necesario, 
	   ya que despues de esta operacion no se debe usar el arbol */
	rw_write_ini(&arb->bloq);
	
	/* Vaciar lo que quede del arbol */
	do {
		DEBUGF("[abm_destruir] act=%p (%s,%d)\n",act,(nodo_hoja(act)?"HOJA":"INTERNO"),getNodeSize(act));

		/* Si el nodo es hoja, limpiar a cañon */
		if (nodo_hoja(act)) {
			DEBUGS("[abm_destruir] - Limpiando nodo hoja");
			/* Tanto los registros */
			for(i=0;i<getNodeSize(act);i++) free(getLeafLink(act,i));
			/* Como el nodo */
			free(act);;
			/* Retroceder */
			DEBUGF("[abm_destruir] - Volviendo a %p\n",ant);
			act=ant;
		}
		/* Si es interno, limpiar de derecha a izquierda hasta que esté vacio */
		else {
			/* si está vacio */
			if (getNodeSize(act) == NODE_MAX_CAP) {
				DEBUGF("[abm_destruir] - Nodo interno vacio, liberando y volviendo a %p\n",act->link);
				ant=act->link; // Retroceder
				free(act);;     // Liberar
				act=ant;       // Hemos usado ant de temporal
			}
			/* Si no está vacio, tachamos el mas a la derecha, y descendemos por el */
			else {
				DEBUGF("[abm_destruir] - Nodo interno no vacio, descendiendo por el enlace %d a %p\n",
					getNodeSize(act),getInnerLink(act,getNodeSize(act)));
				ant=act;
				tmp=getNodeSize(act);
				setNodeSize(act,getNodeSize(act)-1);
				act=getInnerLink(act,tmp);
			}
		}
	} while(act!=NULL);
		
	/* Liberar el arbol */
	rw_write_fin(&arb->bloq);
	free(arb);;
}

/** Inserta un registro en el arbol
    Para insertar un registro en el arbol:
    - Primero se localiza el nodo donde insertarlo.
    - Si el nodo en cuestion está lleno hay que dividirlo, lo que puede 
      encadenar mas de una división.
    - Si no, se reserva memoria compartida para el registro, y se inserta.
    
    \param[in] arb Arbol con el que trabajar.
    \param[in] registro  Zona de memoria con el registro a insertar. Después
                         de la insercion, esta memoria se puede liberar, ya 
			 que se hace copia del contenido.
    \return Un entero con el estado de la operacion.
*/
int abm_insertar(Arbol *arb,void *registro) {
	ABMNodo *destino,*anterior;
	int resultado,pos,arbol_escritura;
	cmpFunc comparar=getCmpFunc(arb);

	DEBUGF("[abm_insertar](arb: %p,registro: %p)\n",arb,registro);

	arbol_escritura=0;
	while(1) {
		/* Sincronizacion del arbol */
		DEBUGF("[abm_insertar] Sincronizacion arbol - escritura:%d\n",arbol_escritura);
		if (arbol_escritura) {
			//printf("[%u][abm_insertar] rw_write_ini arbol (bloq: %p)\n",pthread_self(),&arb->bloq);fflush(NULL);
			rw_write_ini(&arb->bloq);
		}
		else {
			//printf("[%u][abm_insertar] rw_read_ini arbol (bloq: %p)\n",pthread_self(),&arb->bloq);fflush(NULL);
			rw_read_ini(&arb->bloq);
		}
	
		/* Buscar nodo Hoja.
		   Dado un arbol, y un registro obtiene el nodo hoja donde insertar
	 	   el registro.
		*/
		searchNodeLeaf(getRoot(arb),destino,anterior,registro);

		/* Sincronizacion nodo hoja */
		if (arbol_escritura==0) {
			//printf("[%u][abm_insertar] rw_write_ini nodo (bloq: %p)\n",pthread_self(),&destino->bloq);fflush(NULL);
			rw_write_ini(&destino->bloq);
		}
		
		if (nodo_lleno(destino)) {
			ABMNodo *derecho;
		
			DEBUGS("[abm_insertar] Nodo destino lleno");
			
			/* Si el nodo esta lleno y no ibamos a modificar el arbol
			   Volver a empezar y reservar el arbol para escritura ya 
			   que vamos a modificar el arbol al dividir el nodo */
			if (arbol_escritura==0) {
				//printf("[%u][abm_insertar] rw_write_fin nodo\n",pthread_self());fflush(NULL);
				rw_write_fin(&destino->bloq);
				//printf("[%u][abm_insertar] rw_read_fin arbol (bloq: %p)\n",pthread_self(),&arb->bloq);fflush(NULL);
				rw_read_fin(&arb->bloq);
				arbol_escritura=1;
				continue;
			}
		
			/* Si estamos aqui es que arbol_escritura != 0 y podemos modificar el arbol */
			derecho=splitNodeLeaf(arb,destino,anterior);
			
			/* Comparacion para ver donde insertar */
			resultado=comparar(registro,getLastKey(destino));
			if(resultado==1) 
			/* Resulta que la clave de 'registro' es mayor que la 1ª del derecho 
			   Luego toca insertar en el derecho. */
				destino=derecho;
			/* Si no, destino no se toca y se le considera el izquierdo */
		}
		
		resultado=insertNodeLeaf(arb,destino,registro);
		

		/* Fin de uso del arbol */
		if (arbol_escritura) {
			//printf("[%u][abm_insertar] rw_write_fin arbol\n",pthread_self());fflush(NULL);
			rw_write_fin(&arb->bloq);
		}
		else {
			//printf("[%u][abm_insertar] rw_write_fin nodo\n",pthread_self());fflush(NULL);
			rw_write_fin(&destino->bloq);

			//printf("[%u][abm_insertar] rw_read_fin arbol\n",pthread_self());fflush(NULL);
			rw_read_fin(&arb->bloq);
		}
	
		return resultado;
	} /* Fin del while */
}

/** Inserta un registro en un nodo hoja.
    - Si el nodo está vacío inserta directamente.
    - Si el nodo está lleno: error, debería haberse dividido anteriormente.
      Esto puede entrar un poco en conflicto con la manera de actuar de insertNodeInner, 
      que divide el nodo si está lleno, mientras que aqui tienen que dividirlo antes.
    - En cualquier otro caso:
    	- Se busca la posicion adecuada de inserción.
	- Se desplazan los datos en el registro si es necesario.
	- Se inserta.

    \param[in] arb Arbol donde realizar la operación.
    \param[in] destino Nodo hoja donde insertar.
    \param[in] registro Registro de datos a insertar.
    \return Un entero indicando el estado de la operacion
*/
static int insertNodeLeaf(Arbol *arb,ABMNodo *destino,void *registro) {
	cmpFunc comparar=getCmpFunc(arb);
	int res,desde,i;
	void *nuevoReg;

	DEBUGF("[insertNodeLeaf](arb: %p,destino: %p,registro: %p)\n",arb,destino,registro);
	
	/* Buscar donde insertar */
	if (nodo_vacio(destino)) {
		DEBUGS("[insertNodeLeaf] El nodo esta vacio");
		setNodeSize(destino,1);           // El tamaño del nodo pasa a ser 1
		setNodeKey(destino,0,registro);   // Se pone la clave numero 0
		nuevoReg=malloc(getRegSize(arb)); // Se reserva memoria para el registro
		setLeafLink(destino,0,nuevoReg);  // Se apunta el enlace 0 al nuevo registro
		memcpy(nuevoReg,registro,getRegSize(arb));  // Se copian los datos al nuevo registro.
		DEBUGF("[insertNodeLeaf] nuevoReg:%p bytes:%d\n",getLeafLink(destino,0),getRegSize(arb));	
		/** \retval ABM_OK La inserción se ha realizado correctamente */
		return ABM_OK;
	}
	if (nodo_lleno(destino)) {
		DEBUGS("[insertNodeLeaf] El nodo esta lleno - ERROR");
		/** \retval ABM_ERROR ha ocurrido un error al insertar (el nodo estaba lleno) */
		return ABM_ERROR;
        }
	DEBUGS("[insertNodeLeaf] El nodo no esta ni vacio ni lleno");
	
	desde=buscarClave(comparar,registro,destino);
	res=comparar(registro,getNodeKey(destino,desde));
	/** \retval ABM_DUP El registro ya existía, no se admiten duplicados */
	if(res==0) {
		return ABM_DUP;
	}
	if(res>0) desde++;

	/* Desplazar el resto del nodo a la derecha */
	DEBUGF("[insertNodeLeaf] Moviendo en nodo %p desde:%d hasta:%d\n",destino,desde,getNodeSize(destino)-1);
	for (i=getNodeSize(destino)-1;i>=desde;i--) {
		DEBUGF("[insertNodeLeaf]\t Moviendo de %d(%p) a %d(%p) %d bytes \n",
			i,getNodeKey(destino,i),i+1,getNodeKey(destino,i+1),getKeySize(destino));
		setNodeKey(destino,i+1,getNodeKey(destino,i));
		setLeafLink(destino,i+1,getLeafLink(destino,i));
	}
	/* Insertar */
	setNodeKey(destino,desde,registro);        // Establecer la clave de la posicion "desde"
	nuevoReg=malloc(getRegSize(arb));          // Se reserva memoria para el registro
	setLeafLink(destino,desde,nuevoReg);       // Apuntar el enlace "desde" al nuevo registro
	memcpy(nuevoReg,registro,getRegSize(arb)); // Copiar los datos del registro
	getNodeSize(destino)++;                    // incrementar en 1 el tamaño del nodo
	
	/** \retval ABM_OK La insercion se ha realizado correctamente */
	return ABM_OK;
}

/** Devuelve la posicion del enlace a seguir dada una clave.
    Utiliza una búsqueda binaria para encontrar la posicion adecuada, y se 
    adapta a los nodos internos que poseen un enlace más 
    \param[in] act   Nodo en el cual buscar la posicion.
    \param[in] clave Clave a seguir.
    \return La posicion a seguir.
*/
static int getNextLinkPos(ABMNodo *act,void *clave,cmpFunc comparar) {
	int pos;

        /* Buscar el lugar aproximado mediante una b<FA>squeda binaria inexacta
         * inexacta: si no encuentra el valor exacto devuelve la posicion del m<E1>s cercano
         */	
	pos=buscarClave(comparar,clave,act);

        /* Si es un nodo interno lo mas seguro que no encuentre la clave exacta, por lo que devolver<E1>
         * una aproximada, puede ser la mayor o la menor:
         * - Si la clave es mayor que la clave de la posicion devuelta, el enlace es el derecho (pos+1)
         * -- recordemos que dada una clave, los menores o iguales van a su izquierda.
         * - Si la clave es menor o igual que la clave de la posicion devuelta, el enlace es el izquierdo (pos)
         */
        if(nodo_interno(act) && comparar(clave,getNodeKey(act,pos))>0) return pos+1;
        return pos;     	
}

/** Busqueda binaria en el vector de claves 
     - Se presupone que ocupados >= 1
 
    \param[in] comparar Función que compara 2 claves.
    \param[in] key Puntero a la clave a buscar.
    \param[in] nodo Nodo en el que buscar la clave
    \return Un entero con la posicion del vector mas cercana a la clave buscada.
*/
static int buscarClave(cmpFunc comparar,void *key,ABMNodo *nodo) {
	int ocupados=getNodeSize(nodo);
        int inf=0,sup=ocupados-1,mid=(ocupados-1)>>1;
	int res; ///< Lugar donde almacenar el resultado de la comparacion
	
	DEBUGF("[buscarClave](key: %p,nodo: %p,ocupados: %d,comparar: %p)\n",key,nodo,ocupados,&comparar);
        while(inf<sup) {
                DEBUGF("[buscarClave] inf,mid,sup: %d,%d,%d\n",inf,mid,sup);
		res=comparar(getNodeKey(nodo,mid),key);
		/* clave(mid)>key */
                if (res>0) sup=mid-1;
		/* clave(mid)<key */
                else if (res<0) inf=mid+1;
		/* clave(mid)==key */
                /* Lo encuentra y devuelve el valor */
                else if (res==0) return mid;
                /* No lo ha encontrado pero devuelve el valor mas cercano */
                mid=(inf+sup)>>1;
        }
        DEBUGF("[buscarClave] inf,mid,sup: %d,%d,%d\n",inf,mid,sup);
        return (inf>ocupados)?(inf-1):(inf);
}

static ABMNodo *splitNodeLeaf(Arbol *arb,ABMNodo *actual,ABMNodo *anterior) {
        ABMNodo *izq=actual,*der=NULL;
        int i,j,midVal,actOcupados=0;
	char midKey[getKeySize(actual)];

        DEBUGF("[splitNodeLeaf](arb: %p,actual: %p,anterior:%p)\n",arb,actual,anterior);

        /* Obtenemos la clave del medio del nodo: tanto su posicion como su valor */
        midVal=getNodeSize(actual) >> 1; // Posicion del medio
        memcpy(midKey,getNodeKey(actual,midVal-1),getKeySize(actual)); // valor. midkey=getNodeKey....
	
        /* El nuevo nodo será el de la derecha */
        der=memNuevoNodo(NODE_ATRIB_LEAF,getKeySize(actual));

        /* Actualizar la lista enlazada */
        /* 1.- el de la derecha apunta a donde apuntaba antes el total */
        der->link=izq->link;
        /* 2.- El de la izquierda apunta a su derecha */
        izq->link=der;
        DEBUGF("[splitNodeLeaf] midVal(%d) izquierdo(%p) derecho(%p)\n",midVal,izq,der);

        /* Tamaños */
        actOcupados=getNodeSize(actual);
        /* La parte derecha obtiene la mitad mas el redondeo */
        setNodeSize(der,midVal + actOcupados%2);
        /* La parte izquierda obtiene la otra mitad */
        setNodeSize(izq,midVal);
        DEBUGF("[splitNodeLeaf] izq->ocupados=%d der->ocupados=%d\n",getNodeSize(izq),getNodeSize(der));


        /* Copiamos los datos 
         * i es el indice del origen.
         * j es el indice del destino.
         * copiamos de izq a der (que es el nuevo nodo)
         * */
        DEBUGF("[splitNodeLeaf] copiando al nodo derecho: de %d a %d\n",midVal,actOcupados-1);
        for(i=midVal,j=0;i<actOcupados;i++,j++) {
		setLeafLink(der,j,getLeafLink(izq,i)); // der->enlaces.externo[j]=izq->enlaces.externo[i];
		setNodeKey(der,j,getNodeKey(izq,i));   // der->claves[j]=izq->claves[i];
        }

        /* Insertamos la clave de en medio en el nodo superior */
        insertNodeInner(arb,anterior,izq,der,midKey);

	return der;
}

/** Inserta una clave en un nodo interno.
    Necesita los dos nodos que "cuelgan" de la clave.
*/
static void insertNodeInner(arb,act,izq,der,midKey) 
Arbol*   arb;
ABMNodo  *act,*izq,*der;
char*    midKey;
{
	cmpFunc comparar=getCmpFunc(arb);
        int desde,i,j,dividido=0;
	
        
        DEBUGF("[insertNodeInner](arb:%p,act:%p,izq:%p,der:%p,midKey:%p)\n",arb,act,izq,der,midKey);
        /* Si act es NULL es que izq era el raiz, y al dividirse hace falta un nuevo raiz */
        if (act == NULL) {
                act=memNuevoNodo(NODE_ATRIB_INNER,getKeySize(der)); // Crear el nuevo Raiz
                DEBUGF("[insertNodeInner] Cambiando el nodo raiz de %p a %p\n",getRoot(arb),act);
                setRoot(arb,act);                                   // Asignar el nuevo raiz
                /* Si hemos dividido el nodo raiz, y ademas el nodo dividido era interno, 
                 * el enlace al nodo superior es NULL, por lo que hay que poner los valores 
                 * enlace al nuevo nodo raiz */
                if (nodo_interno(izq)) izq->link=der->link=act;     // Actualizar enlaces al nodo superior
		// Esto anterior quizas sobre, mirar a ver si se actualizan los nodos cuando se divide
        }
        
	/* Caso rápido: el nodo esta vacío */
        if (nodo_vacio(act)) {
                setNodeKey(act,0,midKey); // Insertar la clave en la primera posicion
                setInnerLink(act,0,izq);  // Poner el enlace de la izquierda
                setInnerLink(act,1,der);  // Y luego el de la derecha
                setNodeSize(act,1);       // El tamaño del nodo es ahora 1
                return;
        }

	/* El nodo está lleno: dividir y mas tarde insertar en el lado que corresponda */
        if (nodo_lleno(act)) {
                char claveSeparadora[getKeySize(act)]; // Lugar donde almacenar la clave que actua de divisor
                ABMNodo *nuevoIzq,*nuevoDer;           // Punteros a los nuevos nodos.

                DEBUGF("[insertNodeInner] Nodo lleno %p (%d)\n",act,getNodeSize(act));
                splitNodeInner(arb,act,&nuevoIzq,&nuevoDer,claveSeparadora); // Dividir el nodo actual
                /* Ahora que ya lo hemos separado, podemos insertar */
                DEBUGF("[insertNodeInner] Nodo lleno. Separado. izq:%p der:%p act:%p claveSep:%p\n",
			izq,der,act,claveSeparadora);
                if (getCmpFunc(arb)(midKey,claveSeparadora)>0) {
                        act=nuevoDer;
                        /* el nodo ha sido dividido, como sabemos que siempre el derecho es el nuevo, y dado 
                         * que no estaba enlazado al superior, el superior no ha podido cambiar su padre,
                         * lo cambiamos nosotros aqui. */
                        if (nodo_interno(der)) der->link=act;
                }
        }
        
        /* En este momento ya tenemos el nodo interno en el cual insertar */
        desde=buscarClave(comparar,midKey,act);
        /* Nos aseguramos de que "desde" sea el lugar exacto donde ponerlo ya que
         * Si el numero es menor o igual que la clave de "desde", es correcto, porque lo insertamos a laizquierda
         * Si el numero es mayor que la clave de "desde", entones e suna posicion más a la derecha
         */
        if (comparar(midKey,getNodeKey(act,desde))>0) desde++;
        
        DEBUGF("[insertNodeInner] Asercion: if (act->enlaces.interno[%d]!=izq): ",desde);
#ifdef DEBUG    
        if (getInnerLink(act,desde)!=izq) puts("ERROR!");
        else puts("OK!");       
#endif
	/* Desplazar el resto del nodo a la derecha  (de manera inclinada \ ) */
	DEBUGF("[insertNodeInner] Desplazar desde:%d hasta:%d\n",desde,getNodeSize(act)-1);
        for (i=getNodeSize(act)-1;i>=desde;i--) {
		DEBUGF("[insertNodeInner]\t Moviendo de %d(%p) a %d(%p) %d bytes \n",
			i,getNodeKey(act,i),i+1,getNodeKey(act,i+1),getKeySize(act));
                setNodeKey(act,i+1,getNodeKey(act,i));
                setInnerLink(act,i+2,getInnerLink(act,i+1));

        }
	DEBUGF("[insertNodeInner]\t Clave %p a %d y Enlace a nodo %p en pos %d\n",midKey,i,der,desde+1);
        setNodeKey(act,desde,midKey);  // Ponemos la clave en la posicion desde
        setInnerLink(act,desde+1,der); // Ponemos el enlace al nuevo nodo en desde+1
        getNodeSize(act)++;
}

static void splitNodeInner(arb,act,izq_return,der_return,midKey) 
Arbol   *arb;
ABMNodo *act,**izq_return,**der_return;
char    *midKey;
{
        ABMNodo *izq,*der;
        int midVal,actOcupados,i;
        //char midKey[getRegSize(arb)];

        DEBUGF("[splitNodeInner](arb:%p act:%p izq:%p der:%p midkey_ret:%p)\n",arb,act,izq_return,der_return,midKey);

	/* Obtenemos la clave del medio del nodo: tanto su posicion como su valor */
        midVal=getNodeSize(act) >> 1;                            // Posicion del medio
        memcpy(midKey,getNodeKey(act,midVal),getKeySize(act)); // valor. midkey=getNodeKey....
	DEBUGF("[splitNodeInner] actSize:%d midVal:%d\n",getNodeSize(act),midVal);

        /* El nuevo nodo será el de la derecha */
        der=memNuevoNodo(NODE_ATRIB_INNER,getKeySize(act));

        /* Actualizar punteros */
        izq=act;
        *der_return=der;
        *izq_return=izq;

        /* Actualizar el enlace al padre */
        der->link=izq->link; 
        
        /* Actualizar tamaños */
        actOcupados=getNodeSize(act);
        setNodeSize(izq,midVal);

        /* Cuando el nodo es impar la cantidad de elementos en cada lado es la misma
         * pero cuando es par, hay uno que se queda con un m<E1>s, ya que eliminamos uno */
        setNodeSize(der,midVal - ((actOcupados%2)?0:1));

        DEBUGF("[splitNodeInner] izq->ocupados=%d der->ocupados=%d actOcupados=%d\n",
		getNodeSize(izq),getNodeSize(der),actOcupados)
;       
        /* Copiar datos sin copiar el de enmedio */
        midVal++; // Clave que no se va a copiar
        DEBUGF("[splitNodeInner] Copiando de %d a %d, quitamos la clave: %d\n",midVal,actOcupados-1,midVal-1);
        for (i=0;midVal<actOcupados;midVal++,i++) {
		setNodeKey(der,i,getNodeKey(izq,midVal));     // der->claves[i]=izq->claves[midVal];
                setInnerLink(der,i,getInnerLink(izq,midVal)); // der->enlaces.interno[i]=izq->enlaces.interno[midVal];
        }
	/* El enlace que falta */
	setInnerLink(der,i,getInnerLink(izq,midVal));         // der->enlaces.interno[i]=izq->enlaces.interno[midVal];
        
        /* Si uno de los anteriores es interno, todos los son, al dividir el nodo la 
         * parte derecha tiene nodos colgado cuyo enlace al padre est<E1> mal, ya que la
         * parte derecha es la nueva. Entonces, actualizar esos enlaces al padre*/
        if( nodo_interno(getInnerLink(der,0)) ) {
                for(i=0;i<=getNodeSize(der);i++)
                        getInnerLink(der,i)->link=der;
        }
        
        /* Insertamos la clave en el nodo superior (act->link) */
        insertNodeInner(arb,act->link,izq,der,midKey);
}

/** Busca un registro dada una clave.
    Utiliza el parametro registro tanto para entrada: es ahí donde se indica
    la clave a buscar; como para salida: en caso de encontrar el registro,
    los datos van a parar a esa estructura.

    \param[in] arb Arbol donde realizar la operación.
    \param[in,out] registro Contiene la clave por la que buscar, y almacena el
                            resultado en caso de que la busqueda de resultados.
    \return Indicando si la busqueda ha tenido o no éxito.
*/
int abm_buscar (Arbol *arb,void *registro) {
	cmpFunc comparar=getCmpFunc(arb);
	ABMNodo *actual=getRoot(arb);
	int posicion;

	DEBUGF("[abm_buscar](arb: %p,registro: %p)\n",arb,registro);
	
	/* Sincronizacion del arbol */
	//printf("[%u][abm_buscar] rw_read_ini arbol (bloq: %p)\n",pthread_self(),&arb->bloq);fflush(NULL);
	rw_read_ini(&arb->bloq);
	
	/* Buscar el nodo hoja correspondiente */
	while(nodo_interno(actual))
		actual=getInnerLink(actual,getNextLinkPos(actual,registro,comparar));
	
	/* Sincronizacion nodo hoja */
	//printf("[%u][abm_buscar] rw_read_ini nodo\n",pthread_self());fflush(NULL);
	rw_read_ini(&actual->bloq);
	
	/* Buscar la posicion en el nodo hoja */
	posicion=getNextLinkPos(actual,registro,comparar);
	DEBUGF("[abm_buscar] Posible posicion: %d\n",posicion);

	/* ¿Lo hemos encontrado? */
	if(comparar(getNodeKey(actual, posicion),registro)==0) {
		DEBUGF("[abm_buscar] Dato encontrado en posicion %d\n",posicion);
		/* Copiar los datos */
		memcpy(registro,getLeafLink(actual,posicion),getRegSize(arb));
		/* Devolver ok */
		/** \retval ARB_OK Se ha encontrado el registro */
		//printf("[%u][abm_buscar] rw_read_fin nodo\n",pthread_self());fflush(NULL);
		rw_read_fin(&actual->bloq);
		//printf("[%u][abm_buscar] rw_read_fin arbol\n",pthread_self());fflush(NULL);
		rw_read_fin(&arb->bloq);
		return ABM_OK;
	}
	
	DEBUGS("[abm_buscar] Dato NO encontrado");
	/** \retval ABM_404 Valor no encontrado */
	//printf("[%u][abm_buscar] rw_read_fin nodo\n",pthread_self());fflush(NULL);
	rw_read_fin(&actual->bloq);
	//printf("[%u][abm_buscar] rw_read_fin arbol\n",pthread_self());fflush(NULL);
	rw_read_fin(&arb->bloq);
	
	return ABM_404;
}

/** Modifica un registro dada su clave.
    Modifica el contenido de un registro buscandolo a partir de su clave. Es
    importante señalar, que NO SE DEBE cambiar la clave del registro con esta 
    operación, pues los resultados NO se reflejarán en el arbol y el registro 
    solo se podrá acceder usando la clave anterior.
    \param[in] arb Arbol donde realizar la operación.
    \param[in] registro Registro que contiene tanto la clave como los nuevos 
                        datos a insertar, esto último en el caso de que hacer 
			sea NULL.
    \param[in] hacer Funcion que contiene una operacion a realizar cuando se 
                     encuentre el registro a modificar. Este parametro puede
		     ser NULL, y en este caso se cambiaran los datos del 
		     registro encontrado a los del parametro "registro"; si no 
		     es null, se utilizará la clave del parametro registro,
		     y cuando se encuentre el dato, se aplicará la funcion.
    \return Un entero indicando el resultado de la operación.
*/
int abm_modificar(Arbol *arb,void *registro,doFunc hacer) {
        cmpFunc comparar=getCmpFunc(arb);
        ABMNodo *actual=getRoot(arb);
        int posicion;

        DEBUGF("[abm_modificar](arb: %p,registro: %p)\n",arb,registro);
        /* Sincronizacion del arbol */
	//printf("[%u][abm_modificar] rw_read_ini arbol\n",pthread_self());fflush(NULL);
	rw_read_ini(&arb->bloq);
	
	/* Buscar el nodo hoja correspondiente */
        while(nodo_interno(actual))
                actual=getInnerLink(actual,getNextLinkPos(actual,registro,comparar));

	/* Sincronizacion nodo hoja */
	//printf("[%u][abm_modificar] rw_write_ini hoja\n",pthread_self());fflush(NULL);
	rw_write_ini(&actual->bloq);
	
        /* Buscar la posicion en el nodo hoja */
        posicion=getNextLinkPos(actual,registro,comparar);
        DEBUGF("[abm_modificar] Posible posicion: %d\n",posicion);

        /* ¿Lo hemos encontrado? */
        if(comparar(getNodeKey(actual, posicion),registro)==0) {
                DEBUGF("[abm_modificar] Dato encontrado en posicion %d\n",posicion);
		if(hacer) {
			/* Realizamos la modificacion */
			hacer(registro,getLeafLink(actual,posicion));
			/* Copiamos los cambios */
			memcpy(registro,getLeafLink(actual,posicion),getRegSize(arb));
		}
		else {
                	/* Copiar los datos. Solo los datos, ya que aunque si encontramos la clave
			   al copiarla, seria copiar lo mismo; esta suposicion se basa en que la
			   funcion de busqueda realize su trabajo correctamente */
                	memcpy((void*)getLeafLink(actual,posicion)+getKeySize(actual),
				registro+getKeySize(actual),getRegSize(arb)-getKeySize(actual));
		}
                /* Devolver ok */
                /** \retval ARB_OK Se ha encontrado y modificado el registro */
		//printf("[%u][abm_modificar] rw_write_fin hoja\n",pthread_self());fflush(NULL);
		rw_write_fin(&actual->bloq);
		//printf("[%u][abm_modificar] rw_read_fin arbol\n",pthread_self());fflush(NULL);
		rw_read_fin(&arb->bloq);
                return ABM_OK;
        }

        DEBUGS("[abm_modificar] Dato NO encontrado");
        /** \retval ABM_404 Valor no encontrado, por lo que no se pudo modificar */
	//printf("[%u][abm_modificar] rw_write_fin hoja\n",pthread_self());fflush(NULL);
        rw_write_fin(&actual->bloq);
	//printf("[%u][abm_modificar] rw_read_fin arbol\n",pthread_self());fflush(NULL);
	rw_read_fin(&arb->bloq);
	return ABM_404;	
}

/** Inicializa un nuevo iterador.
    Dado un arbol, crea un nuevo iterador para recorrerlo secuencialmente
    en busca de registros que encajen en las condiciones que aplicará la
    funcion "encaja".
    \param[in] it  Iterador a inicializar.
    \param[in] arb Arbol que recorrer.
    \param[in] encaja Funcion de encaje. Su funcion consiste en que dado un
                      registro devuelve 1 si cumple ciertas condiciones que
                      tiene la funcion internamente, o 0 si no las cumple.
    \param[in] datos Informacion extra para que la funcion encaje trabaje.
    \return Un nuevo iterador listo para usarse al principio del arbol.
*/
void abm_it_init (iterador *it,Arbol *arb,itFunc encaja,void *datos) {
        DEBUGF("[abm_it_init](iterador: %p, Arbol: %p, itFunc: %p, datos: %p)",
	it,arb,encaja,datos);
	
        /* Rellenar los datos del iterador */
        it->actual=arb->listaHojas;
        it->pos=-1;
        it->regsize=getRegSize(arb);
        it->encaja=encaja;
        it->datos=datos;
        it->bloq=&arb->bloq; /* Bloqueo del arbol*/
	
	/* Dejamos el arbol bloqueado en modo lectura */
	rw_read_ini(it->bloq);
}

/** Pone fin a una iteracion por los nodos hoja de un arbol
*/
void abm_it_fin(iterador *it) {
	DEBUGF("[abm_it_fin](iterador: %p)\n",it);
	
	/* Desbloqueamos el arbol ya que no vamos a iterar más */
	rw_read_fin(it->bloq);
}

int abm_iterar(iterador *itr,void *registro) {
	int i;
	void *regNodo;
	ABMNodo *anterior;

	DEBUGF("[abm_iterar](iterador: %p,registro: %p)\n",itr,registro);
	anterior=NULL;
	
	/* Avanzar y devolver datos */
	while(itr->actual!=NULL) {
		/* Liberar el bloqueo del nodo anterior
		   Posibilidad de interbloqueo -> No, ya que el acceso es siempre 
		   en la misma direccion: izquierda a derecha */
		if (anterior) {
			//printf("[%u][abm_iterar] rw_read_fin nodo anterior (bloq: %p)\n",pthread_self(),&anterior->bloq);
			//fflush(NULL);
			rw_read_fin(&anterior->bloq);
		}
		/* Lector en el nodo actual */
		//printf("[%u][abm_iterar] rw_read_ini nodo (bloq: %p)\n",pthread_self(),&itr->actual->bloq);fflush(NULL);
		rw_read_ini(&itr->actual->bloq);


		DEBUGF("[abm_iterar] Nodo actual: %p tam: %d\n",itr->actual,getNodeSize(itr->actual));
		/* Avanzar lo que queda de nodo */
		for(i=itr->pos+1;i<getNodeSize(itr->actual);i++) {
			DEBUGF("[abm_iterar] Posicion %d\n",i);
			regNodo=getInnerLink(itr->actual,i);
			/* Utilizamos la funcion de "mirar si encaja" */
			if (itr->encaja(regNodo,itr->datos)==1) {
				/* Copiamos los datos */
				DEBUGF("[abm_iterar] Dato: %p destino: %p tam: %d\n",regNodo,registro,itr->regsize);
				memcpy(registro,regNodo,itr->regsize);
				/* Actualizar posicion */
				itr->pos=i;
				/** \retval ABM_OK Hemos encontrado un dato */
				//printf("[%u][abm_iterar] rw_read_fin nodo actual (bloq: %p)\n",pthread_self(),&itr->actual->bloq);
				//fflush(NULL);
				rw_read_fin(&itr->actual->bloq);
				return ABM_OK;
			}
		}
		/* Pasar al siguiente nodo */
		anterior=itr->actual;
		itr->actual=itr->actual->link;
		itr->pos=-1;
	}

	/* Si estamos aqui es que hemos llegado al final */
	/** \retval ABM_FIN No hemos encontrado datos que encajen con la funcion indicada */
	if (anterior) {
		//printf("[%u][abm_iterar] rw_read_fin nodo anterior (bloq: %p)\n",pthread_self(),&anterior->bloq);fflush(NULL);
		rw_read_fin(&anterior->bloq);
	}
	
	return ABM_FIN;
}

/** Borra un registro del arbol.
    El borrado de un registro del arbol es una de las operaciones mas costosas
    debido a que puede implicar desde el simple borrado del registro, balanceo 
    entre hojas, hasta borrado de nodos y reajuste del arbol.
    \param[in] arb Arbol donde realizar las operaciones.
    \param[in] clave Clave del registro a borrar. 
    \return Un entero indicando el resultado de la operación.
*/
int abm_borrar (Arbol *arb,void *clave) {
	int aviso=0;
	
	DEBUGF("[abm_borrar](arb: %p, clave: %p)\n",arb,clave);
	
	//printf("[%u][abm_borrar] rw_read_ini arbol\n",pthread_self());fflush(NULL);
	rw_read_ini(&arb->bloq);
	deleteDescend(arb,clave,getRoot(arb),NULL,NULL,NULL,NULL,NULL,&aviso);
	//printf("[%u][abm_borrar] rw_read_fin arbol\n",pthread_self());fflush(NULL);
	rw_read_fin(&arb->bloq);

	if (aviso) {
		DEBUGS("[abm_borrar] Necesito bloquear el arbol para escritura");
		rw_write_ini(&arb->bloq);
		deleteDescend(arb,clave,getRoot(arb),NULL,NULL,NULL,NULL,NULL,&aviso);
		rw_write_fin(&arb->bloq);
	}
	
	/* De primeras devolvemos ok */
	return ABM_OK;
}

/** Desciende por el arbol para borrar.
*/
static ABMNodo *deleteDescend(arb,key,act,izq,der,ant,iant,rant,aviso_modificacion) 
Arbol *arb;
void *key;
ABMNodo  *act, *izq, *der, *ant, *iant, *rant;
int *aviso_modificacion;
{
	int pos;
	ABMNodo *nextAct,*nextLeft,*nextRight,*nextLAnt,*nextRAnt;
	ABMNodo *res=NULL;
	cmpFunc comparar=getCmpFunc(arb);
	
	DEBUGF("[deleteDescend] (arb:%p key:%p act:%p izq:%p der:%p ant:%p iant:%p rant:%p\n", 
	       arb, key, act, izq, der, ant, iant, rant);
	
	/* Cogemos el indice del enlace siguiente que debieramos seguir para descender */
	if (nodo_hoja(act) && (*aviso_modificacion)==0) {
		/* Si es un nodo hoja y no hemos bloqueado el arbol para escritura
		Hay que bloquear el nodo hoja para lectura ya que buscamos en su interior */
		rw_read_ini(&act->bloq);
		pos=getNextLinkPos(act,key,comparar);
		rw_read_fin(&act->bloq);
	}
	else pos=getNextLinkPos(act,key,comparar);
		
	if(nodo_interno(act)) {
		DEBUGS("[deleteDescend] El nodo es interno -> Prepararse para descender correctamente");
		nextAct=getInnerLink(act,pos);
		/* Obtener los enlaces a los nodos izquierdo y derecho */
		/* Empezamos por el de la izquierda, especial atencion a iant */
		if (pos==0) {
			/* Estamos al borde izquierdo de un nodo */
			if (izq!=NULL) {
				nextLeft=getInnerLink(izq,0);
				nextLAnt=izq;
			}	
			else nextLeft=nextLAnt=NULL;
		}
		/* sino, pues caso normal, izquierda = una posicion menos */
		else {
			nextLeft=getInnerLink(act,pos-1);
			nextLAnt=act;
		}
		/* segimos con la derecha, especial atecion a rant */
		if (pos==getNodeSize(act)) {
			if (der!=NULL) {
				nextRight=getInnerLink(der,getNodeSize(der));
				nextRAnt=der;
			}	
			else nextRight=nextRAnt=NULL;
		}
		/* caso normal, donde derecha = una poscion más) */
		else {
			nextRight=getInnerLink(act,pos+1);
			nextRAnt=act;
		}
		DEBUGF("[deleteDescend] nextLeft:%p nextRight:%p\n",nextLeft,nextRight);
		/* Si el nodo es interno, seguir buscando */
		res=deleteDescend(arb,key,nextAct,nextLeft,nextRight,act,nextLAnt,nextRAnt,aviso_modificacion);
		/* si nos dicen que volvemos sin hacer nada... volvemos nosotros tb indicando lo mismo */
		if (!res) return 0;
		DEBUGF("[deleteDescend] res=%p\n",res);
	}
	/* Si no estamos en un nodo interno -> estamos en un nodo hoja.
	 * Si no encontramos nada en el nodo hoja, nuestra busqueda ha lelgado a su fin.
	 */
	else {
		//printf("[%u][deleteDescend] rw_write_ini nodo\n",pthread_self());
		if ((*aviso_modificacion)==0) rw_write_ini(&act->bloq);
		if (comparar(key,getNodeKey(act,pos)) != 0) {
			//printf("[%u][deleteDescend] rw_write_fin nodo\n",pthread_self());
			if ((*aviso_modificacion)==0) rw_write_fin(&act->bloq);
			return 0;
		}
	}	
	

	/* Si hemos llegado aqui es que
	 * - Estamos en un nodo hoja y hay que borrar una clave 
	 * - No estamos en un nodo hoja, y se ha hecho un merge de algun nodo descendiente.
	 */

	/* Si el nodo es interno, hay que mirar el valor de res para identificar la posicion del nodo
	 * a borrar, ya que, podremos haber borrado el derecho o el actual al hacer un merge (en el merge 
	 * todo va a la izquierda *

	 * Si nos dicen que el actual ha sido borrado, pos se mantiene tal cual.
	 * Si no, es que ha sido borrado el de la derecha, ya que todo va hacia el nodo izquierdo */
	if (nodo_interno(act) && getInnerLink(act,pos)!=res) pos++;

	DEBUGF("[deleteDescend] Zona de \"Hacer algo\" - Nodo %s - pos:%d\n",nodo_interno(act)?"Interno":"Hoja",pos);	
	/* Si no es un nodo interno, es que es uno hoja y hay que borrar el enlace de la posicion indicada */
	/* Tambien es aqui donde decidimos:
	   - Si es un nodo hoja y al borrar la posicion hay que modificar el resto del arbol.
	   - Si aun siendo hoja, no ocurre nada.... continuar */
	if((*aviso_modificacion)==0 && nodo_hoja(act)) {
		/* Aqui no se entra si se va a modificar el arbol */
		/* ¿ Tenemos que modificar el arbol ? */
		if ((act->ocupados-1) < NODE_SIZE_MIN) *aviso_modificacion=1;
		
		if ((*aviso_modificacion)==0) { 
			/* No hace falta volver podemos borrar directamente */
			removePos(act,pos);
			DEBUGS("[deleteDescend] Despues de borrar no hay que hacer nada mas. volviendo.");
		}
		/* Lo iniciamos despues del else */
		rw_write_fin(&act->bloq);
		return 0;
	}
	/* Si no es un nodo hoja o aun siendolo aviso_modificacion es uno, 
	   es que el arbol esta bloqueado para escritura... continuemos */
	removePos(act,pos);
	
	

	/* Si despues de borrar no hay que hacer nada.... pues no se hace nada */
	if (!nodo_pocos(act)) {
		DEBUGS("[deleteDescend] Despues de borrar no hay que hacer nada mas. volviendo.");
		return 0;
	}
	DEBUGF("[deleteDescend] Despues de borrar el nodo actual tiene pocos elementos: act:%d min:%d\n",
		getNodeSize(act),NODE_SIZE_MIN);
	if(!izq && !der) {
		/* Nodo raiz */
		DEBUGS("[deleteDescend] Hay que operar con el nodo raiz");
		/* El comportamiento del nodo raiz es especial, ya que no podemos hacer merge ni balance 
		 * con ningun otro nodo, asi que solo podemos esperar a que le reste un misero enlace
		 * para sustituirle por el nodo al que enlaza, y asi hasta que ese nodo sea hoja y ya no
		 * podamos ahcer nada mas que dejarle a cero */
		 if (getNodeSize(act)==0 && nodo_interno(act)) {
		 	/* Cambiar el raiz, realmente no es cero del todo. */
		 	DEBUGF("[deleteDescend] Cambiando raiz: viejo:%p",getRoot(arb));
			setRoot(arb,getInnerLink(act,0));
			DEBUGF(" nuevo:%p\n",getRoot(arb));
			memFree(act);
		}
		 return 0;
	}
	/* Si no hay parte de la derecha */
	if(der==NULL) {
		/* Si la parte izquierda puede admitir un enlace sobrante */
		if (nodeCanMerge(izq)) {
			DEBUGS("[deleteDescend] Decision: no hay parte derecha y la izquierda puede admitir");
			res=nodeMerge(izq,act,ant,comparar);
		}
		/* Si no lo puede admitir */
		else {
			DEBUGS("[deleteDescend] Decision: no hay parte derecha y la izquierda esta llena");
			res=nodeBalance(izq,act,ant,comparar);	
		}
	}
	/* Si no hay parte de la izquierda */
	else if(izq==NULL) {
		/* Si la parte derecha puede adminitr un enlace sobrante */
		if (nodeCanMerge(der))  {
			DEBUGS("[deleteDescend] Decision: no hay parte izquierda y la derecha puede admitir");
			DEBUGF(" - der:%d act:%d\n",getNodeSize(der),getNodeSize(act));
			res=nodeMerge(act,der,ant,comparar);
		}
		/* Si no lo puede admitir */
		else {
			DEBUGS("[deleteDescend] Decision: no hay parte izquierda y la derecha esta llena");
			res=nodeBalance(act,der,ant,comparar);
		}
	}
	/* Hay parte izquierda y derecha */
	else {
		/* Si estan juntitos derecha y actual */
		if (rant==ant && iant!=ant) {
			/* Si la parte derecha puede adminitr un enlace sobrante */
			if (nodeCanMerge(der))  {
				DEBUGS("[deleteDescend] Decision: Hay izq y der, der y act mismo padre, der puede admitir");
				res=nodeMerge(act,der,ant,comparar);
			}
		        /* Si no lo puede admitir */
		        else {
				DEBUGS("[deleteDescend] Decision: Hay izq y der, der y act mismo padre, der esta lleno");
				res=nodeBalance(act,der,ant,comparar);
			}
		}
		/* Si estan juntitos izquierda y actual */
		else if (iant==ant && rant!=ant) {
			/* Si la parte derecha puede adminitr un enlace sobrante */
	                if (nodeCanMerge(izq))  {
				DEBUGS("[deleteDescend] Decision: Hay izq y der, izq y act mismo padre, izq puede admitir");
				res=nodeMerge(izq,act,ant,comparar);
			}
	                /* Si no lo puede admitir */
	                else {
				DEBUGS("[deleteDescend] Decision: hay izq y der, izq y act mismo padre, izq esta lleno");
				res=nodeBalance(izq,act,ant,comparar);
			}
		}
		/* si tanto izquierda como derecha estan colgando del mismo nodo que act */
		else {
			/* Si ambos pueden admitir un merge hacer merge al derecho */
			if(nodeCanMerge(izq) && nodeCanMerge(der)) {
				DEBUGS("[deleteDescend] Decision: hay izq y der, todos mismo padre, i&d pueden admitir");
				res=nodeMerge(act,der,ant,comparar);
			}
			/* Sino, es que uno está lleno, balancear el lleno con el vacio*/
			else if (!nodeCanMerge(izq)) {
				DEBUGS("[deleteDescend] Decision: hay izq y der, todos mismo padre, izq esta lleno");
				res=nodeBalance(izq,act,ant,comparar);
			}
			else {
				DEBUGS("[deleteDescend] Decision: hay izq y der, todos mismo padre, der esta lleno");
				res=nodeBalance(act,der,ant,comparar);
			}
		}
	} /* fin else hay parte izquierda y derecha */
	return res;
}

static void removePos(ABMNodo *act,int pos) {
	int i;
	void **node_links=NULL;

	/* Para nodos hoja */
	initSetLink(act);
	DEBUGF("[removePos] act:%p pos:%d link:%p\n",act,pos,getLink(act,pos));

	/* Un poco de "por favor" - Comprobacion antes de borrar */
	/* 1) Si el nodo es interno y borramos un enlace mas allá de la
	      ocupacion real, no hacer nada */
	if (nodo_interno(act) && pos > getNodeSize(act)) 
	{
		DEBUGF("[removePos] Borrando posicion %d de un nodo interno con ocupacion %d\n",pos,getNodeSize(act));
		
		#ifdef __PERFECT_DELETE_
		exit(1);
		#endif
		
		return;
	}
	/* 2) Si el nodo es interno la misma comprobacion de antes pero 
	      teniendo en cuenta que un nodo hoja tiene un enlace menos */
	if (nodo_hoja(act) && pos >= getNodeSize(act)) {
		DEBUGF("[removePos] Borrando posicion %d de un nodo hoja con ocupacion %d\n",pos,getNodesize(act));
		
		#ifdef __PERFECT_DELETE_
		exit(1);
		#endif
		
		return;
	}

	/* Liberar la memoria asociada al enlace */
	if(nodo_interno(act)) memFree(getInnerLink(act,pos));
	else memFree(getLeafLink(act,pos));
	
	/* Desplazar los elementos restantes por si al eliminar se ha hecho un hueco */
	for (i=pos+1;i<getNodeSize(act);i++) {
		setNodeKey(act,i-1,getNodeKey(act,i));
		setLink(i-1,getLink(act,i));
	}
	/* Si el nodo es interno mover el ultimo enlace */
	if (nodo_interno(act) && pos<getNodeSize(act)) setLink(i-1,getLink(act,i));
	/* Si el nodo es interno y pos==nodeSize(act) es simplemente restar uno a capacidad */
	act->ocupados--;
}

static ABMNodo *nodeBalance(ABMNodo *izq,ABMNodo *der,ABMNodo *ant,cmpFunc comparar) {
	int cant,i,j,oldAntPos,mod=0;
	void **node_links;
	ABMNodo *tmp;

	/* Clave Temporal - Inicio */
	char joinKey[getKeySize(izq)];
	char antKey[getKeySize(izq)];
	bzero(joinKey,getKeySize(izq));
	bzero(antKey,getKeySize(izq));

	DEBUGF("[nodeBalance] izq:%p der:%p ant:%p\n",izq,der,ant);
	/* Guardamos la posicio que nos llevaba al nodo izquierdo */
	if(nodo_vacio(izq)) oldAntPos=getNextLinkPos(ant,getFirstKey(der),comparar)-1;
	else oldAntPos=getNextLinkPos(ant,getLastKey(izq),comparar);
	
	/* Si cuando dividiamos un nodo interno desaparecia una clave, que iba a parar al nodo
	 * superior; ahora tenemos que ir al nodo superior a buscarla */
	if (nodo_interno(izq)) memcpy(joinKey,getNodeKey(ant,oldAntPos),getKeySize(izq));
	
	DEBUGF("[nodeBalance] oldAntPos:%d joinKey:%p\n",oldAntPos,joinKey);
	
	/* Balanceamos la carga de los nodos */
	if (getNodeSize(izq)>getNodeSize(der)) {
		DEBUGS("[nodeBalance] El nodo izquierdo es mas grande");
		/* Si el nodo izquierdo es mas grande que el derecho, movemos datos al derecho */
		/* Calculamos cuanto hay que mover (a - b)/2 */
		cant=(getNodeSize(izq)-getNodeSize(der))>>1;
		DEBUGF("[nodeBalance] (%d|%d) mover %d del izquierdo al derecho\n",getNodeSize(izq),getNodeSize(der),cant);
		/* hacemos hueco en el nodo derecho, moviendo los datos a la derecha */
		if (nodo_interno(der)) {
			/* Si es interno, copiamos el enlace extra ultimo */
			mod=1;
			DEBUGF("[nodeBalance] Moviendo link:%p(%d) en derecho a posicion %d\n"
				,getInnerLink(der,getNodeSize(der)),getNodeSize(der),getNodeSize(der)+cant);
			setInnerLink(der,getNodeSize(der)+cant,getInnerLink(der,getNodeSize(der)));
		}
		/* movemos los datos a la derecha */
		initSetLink(der);
		for(i=getNodeSize(der)-1;i>=0;i--) {
			setNodeKey (der,i+cant,getNodeKey (der,i));
			setLink(i+cant,getLink(der,i)); /* setLink(der) */
		}
		/* Clave nueva para el nodo superior
		 * - Si el nodo es interno, al mover los datos a la derecha, hay una clave que del
		 * nodo izquierdo que no es movida: nodeSize(izq)-cant 
		 * - Si el nodo es hoja, hay que cojer la ultima clave del nodo izquierdo, y dado que
		 * empezamos a contar desde cero es: nodeSize(izq)-cant-1
		 */
		memcpy(antKey,getNodeKey(izq,getNodeSize(izq)-cant-1+mod),getKeySize(izq));
		/* copiamos datos del izquierdo al derecho */
		if (nodo_interno(der)) {
			/* Si es interno, copiar el enlace extra */
			tmp=getInnerLink(izq,getNodeSize(izq));
			if (nodo_interno(tmp)) tmp->link=der;
			setInnerLink(der,cant-1,tmp); /* setLink(der) */
			setNodeKey (der,cant-1,joinKey);
		}
		/* Copiamos el resto */
		/* Hay que tener en cuenta que si el nodo no es interno no se hace la copia extra
		del ultimo enlace y por lo tanto no hace falta restar 2 a cant sino simplemente 1 */
		for(i=getNodeSize(izq)-1,j=cant-1-mod;j>=0;j--,i--) {
			tmp=getNodeKey(izq,i);
			if (nodo_interno(izq) && nodo_interno(tmp)) tmp->link=der;
			setNodeKey (der,j,tmp);
			setLink(j,getLink(izq,i)); /* setLink(der) */
		}
	}
	else {
		DEBUGS("[nodeBalance] El nodo derecho es mas grande o igual al izquierdo");
		/* el nodo derecho es mas grande que el izquierdo */
		/* Calcular cuantos tenemos que desplazar de derecha a izquierda */
		cant=(getNodeSize(der)-getNodeSize(izq))>>1;
		DEBUGF("[nodeBalance] (%d|%d) mover %d del derecho al izquierdo\n",getNodeSize(izq),getNodeSize(der),cant);
		/* Si el nodo es interno ponemos la clave que falta encima del ultimo enlaze del izquierdo */
		if (nodo_interno(der)) {
			mod=1;
			setNodeKey(izq,getNodeSize(izq),joinKey);
			tmp=getInnerLink(der,0);
			if (nodo_interno(tmp)) tmp->link=izq;
			setInnerLink(izq,getNodeSize(izq)+1,tmp);
		}
		/* Si hay que copiar mas de uno, ya todo es automático */
		initSetLink(izq);
		for(i=getNodeSize(izq)+mod,j=0;j<(cant-mod);j++,i++) {
			setNodeKey (izq,i,getNodeKey(der,j));
			tmp=getLink(der,j+mod);
			if(nodo_interno(izq) && nodo_interno(tmp)) tmp->link=izq;
			setLink(i+mod,tmp); /* setLink(izq) */
			
		}
		/* Clave superior nueva
		 * - Si el nodo es interno se coje la clave que se va a perder.
		 * - Si el nodo es hoja, se coje la ultima del izquierdo 
		 */
		if (nodo_interno(der)) 
			memcpy(antKey,getNodeKey(der,cant-1),getKeySize(der));
		else    memcpy(antKey,getNodeKey(izq,getNodeSize(izq)+cant-1),getKeySize(der));

		/* Desplazar lo que reste del nodo derecho al principio */
		initSetLink(der);
		for(i=0,j=cant;i<(getNodeSize(der)-cant);i++,j++) {
			setNodeKey(der,i,getNodeKey(der,j));
			setLink(i,getLink(der,j)); /* setLink(der) */
		}
		/* Y si el nodo es interno, solo nos queda el ultimo enlace */
		if (nodo_interno(der)) setLink(i,getLink(der,j)); /* setLink(der) */
		/* Para que funcione bien la cuenta de ocupados */
		cant=(-cant);
	}
	/* Para los dos casos */
	der->ocupados+=cant;
	izq->ocupados-=cant;
	/* actualizamos la clave superior */
	setNodeKey(ant,oldAntPos,antKey);
	
	return 0;
}


static ABMNodo *nodeMerge(ABMNodo *izq,ABMNodo *der,ABMNodo *ant,cmpFunc comparar) {
	int i,j=getNodeSize(izq),mod=0;
	int oldAntPos;
	void **node_links=NULL;
	
	DEBUGF("[nodeMerge] izq:%p(%d) der:%p(%d) ant:%p Nivel:%s\n",
		izq,getNodeSize(izq),der,getNodeSize(der),ant,nodo_interno(izq)?"Interno":"Hoja");
	/* Pasamos todo al nodo izquierdo 
	- Esto funciona aunque el tamaño del nodo sea cero, que en el caso de los internos
	  puede suponer que aun reste un enlace por mover
	*/

	/* hay que guardar el enlace por donde lelgamos al izquierdo para corregirlo mas tarde en
	 * el nodo anterior */
	if (nodo_vacio(der))
		oldAntPos=getNextLinkPos(ant,getLastKey(izq),comparar);
	else
		oldAntPos=getNextLinkPos(ant,getFirstKey(der),comparar)-1;
		
	DEBUGF("[nodeMerge] get%s - oldAntPos:%d\n",nodo_vacio(der)?"LastKey(izq)":"FirstKey(der)",oldAntPos);;
	if (nodo_interno(izq)) {
		/* Si es nodo interno, como movemos al izquierdo, nos hace falta una clave de
		 * enlace a situar encima del ultimo enlace del izquierdo */
		mod=1;
		DEBUGF("[nodeMerge] Nodo Interno: enlace de la posicion %d:%p\n",
			getNodeSize(izq),getInnerLink(izq,getNodeSize(izq)));
		setNodeKey(izq,getNodeSize(izq),getNodeKey(ant,oldAntPos));
		/* Añadimos el enlace para dejar el nodo bien */
                setInnerLink(izq,getNodeSize(izq)+1,getInnerLink(der,0));
		DEBUGF("[nodeMerge] Nodo Interno: moviendo clave (pos:%d -> pos:%d)\n",oldAntPos,getNodeSize(izq));
		DEBUGF("[nodeMerge] Nodo Interno: moviendo enlace %p (pos:%d -> pos:%d)\n",
			getInnerLink(der,0),0,getNodeSize(izq)+1);
        }
	/* Si queda algo mas que pasar, lo terminamos de pasar */						
	initSetLink(izq);
	for (i=getNodeSize(izq)+mod,j=0;j<getNodeSize(der);i++,j++) {
		setNodeKey(izq,i,getNodeKey(der,j));
		setLink(i+mod,getLink(der,j+mod)); /* setLink(izq) */
		DEBUGF("[nodeMerge] Nodo: moviendo clave %p (pos:%d -> pos:%d)\n",getNodeKey(der,j),j,i);
		DEBUGF("[nodeMerge] Nodo: moviendo enlace %p (pos:%d -> pos:%d)\n",getLink(der,j+mod),j+mod,i+mod);
	}
	if(mod) izq->ocupados++;
	izq->ocupados+=der->ocupados;
	der->ocupados=0;
	DEBUGF("[nodeMerge] Izquierda ocupados: %d\n",izq->ocupados);
	
	/* Actualizar lista enlazada */
	if (nodo_hoja(izq)) izq->link=der->link;
	/* Si no, actualizar el enlace al padre de los hijos */
	else if (nodo_interno(getInnerLink(izq,0))) {
		DEBUGS("[nodeMerge] El nodo izq es interno y dependen mas internos de el, arreglando enlaces");
		for(i=0;i<=getNodeSize(izq);i++) {
			DEBUGF("[nodeMerge] Arreglando padre del nodo %p\n",getInnerLink(izq,i));
			getInnerLink(izq,i)->link=izq;
		}
	}
	/* Actualizar la clave del nodo superior */
	if (oldAntPos<NODE_SIZE-1) setNodeKey(ant,oldAntPos,getNodeKey(ant,oldAntPos+1)); 
	
	return der;
}


