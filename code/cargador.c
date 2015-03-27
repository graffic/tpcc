#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#define DEBUG
#include "cargador.h"
#include "generador.h"
#include "debug.h"
#include "arbolbmas.h"
#include "comparadores.h"
#include "registros.h"
#include "listaenlazada.h"

#define CASE_WHILE_LE(readstring,readparam,puntero) \
				while(fscanf(entrada,readstring,readparam)>1) {\
					le_insertar_final((ListaEnlazada*)arb,puntero);\
					contador++;\
				}\
				break;


#define CASE_WHILE(readstring,readparam,puntero) \
				while(fscanf(entrada,readstring,readparam)>1) {\
					abm_insertar(arb,puntero);\
					contador++;\
				}\
				break;




void cargador(BBDD *bd,char *nombre_base) {
	FILE *entrada;
	Arbol *arb;
	char filename[512];
	
	cmpFunc  funcs[]={(cmpFunc)&comparar_productos,(cmpFunc)&comparar_almacenes,   (cmpFunc)&comparar_existencias,
			  (cmpFunc)&comparar_zonas,    (cmpFunc)&comparar_clientes,    (cmpFunc)&comparar_historico,
			  (cmpFunc)&comparar_pedidos,  (cmpFunc)&comparar_lineaspedido,(cmpFunc)&comparar_nuevospedidos};
	int   regsizes[]={sizeof(RegProducto),sizeof(RegAlmacen),    sizeof(RegExistencias),
			  sizeof(RegZona),    sizeof(RegCliente),    sizeof(RegHistorico),
			  sizeof(RegPedido),  sizeof(RegLineaPedido),sizeof(RegNuevoPedido)};
	int   keysizes[]={REGPRODUCTO_KEYSIZE,REGALMACEN_KEYSIZE,    REGEXISTENCIAS_KEYSIZE,
			  REGZONA_KEYSIZE,    REGCLIENTE_KEYSIZE,    REGHISTORICO_KEYSIZE,
			  REGPEDIDO_KEYSIZE,  REGLINEAPEDIDO_KEYSIZE,REGNUEVOPEDIDO_KEYSIZE};
	int i;
	char *ficheros[]={FICHERO_PRODUCTOS,FICHERO_ALMACENES,   FICHERO_EXISTENCIAS,
	                  FICHERO_ZONAS,    FICHERO_CLIENTES,    FICHERO_HISTORICO,
			  FICHERO_PEDIDOS,  FICHERO_LINEASPEDIDO,FICHERO_NUEVOSPEDIDOS};
	Arbol **arboles=(Arbol**)bd;
	union u_reg {
		RegProducto   pr;
		RegAlmacen     al;
		RegExistencias ex;
		RegZona        zo;
		RegCliente     cl;
		RegHistorico   hi;
		RegPedido      pe;
		RegLineaPedido lp;
		RegNuevoPedido np;
	}reg;

	/* Comprobacion inicial ante desbordamientos */
	i=strlen(nombre_base);
	if (i>=494) {
		fprintf(stderr,"[cargador] ERROR Ruta de acceso demasiado larga: \"%s\"\n",nombre_base);
		exit(1);
	}
	
	for(i=0;i<9;i++) {
		int contador=0;
		
		DEBUGF("[cargador] Cargando fichero %s\n",ficheros[i]);
		/* Abrir el fichero i */
		strcpy(filename,nombre_base);
		strcat(filename,ficheros[i]);
		if ((entrada=fopen(filename,"r"))==NULL) {
			perror("");
			fprintf(stderr,"[cargador] ERROR No puedo cargar \"%s\" (%d)\n",filename,i);
			exit(1);
		}
		
		/* Crear un arbol donde almacenarlo */
		if (i==5) {
			DEBUGF("[cargador] Creando Lista enlazada para Historico - regsize:%d\n",regsizes[i]);
			arb=(Arbol*)le_nueva(regsizes[i]);
			DEBUGF("[cargador] Lista creada: %p\n",arb);
		}
		else {
			DEBUGF("[cargador] Creando arbol - regsize:%d keysize:%d cmpfunc:%p\n",
				regsizes[i],keysizes[i],funcs[i]);
			arb=abm_nuevo(regsizes[i],keysizes[i],funcs[i]);
			DEBUGF("[cargador] Arbol creado: %p\n",arb);
		}
		DEBUGS("[cargador] Leyendo datos del fichero");
		/* Leer y cargar el contenido del fichero i */
		switch(i) {
			/* Productos */
			case 0: CASE_WHILE(READSTRING_PRODUCTO,   READPARAM_PRODUCTO(reg.pr),   &reg.pr)
			/* Almacenes */
			case 1: CASE_WHILE(READSTRING_ALMACEN,    READPARAM_ALMACEN(reg.al),    &reg.al) 
			/* Existencias */
			case 2: CASE_WHILE(READSTRING_EXISTENCIAS,READPARAM_EXISTENCIAS(reg.ex),&reg.ex)
			/* Zonas */
			case 3: CASE_WHILE(READSTRING_ZONA,       READPARAM_ZONA(reg.zo),       &reg.zo)
			/* Clientes */
			case 4: CASE_WHILE(READSTRING_CLIENTE,    READPARAM_CLIENTE(reg.cl),    &reg.cl)
			/* Historico */
			case 5: CASE_WHILE_LE(READSTRING_HISTORICO, READPARAM_HISTORICO(reg.hi),&reg.hi)
			/* Pedido */
			case 6: CASE_WHILE(READSTRING_PEDIDO,     READPARAM_PEDIDO(reg.pe),     &reg.pe)
			/* Linea Pedido */
			case 7: CASE_WHILE(READSTRING_LINEAPEDIDO,READPARAM_LINEAPEDIDO(reg.lp),&reg.lp)
			/* Nuevo Pedido */
			case 8: CASE_WHILE(READSTRING_NUEVOPEDIDO,READPARAM_NUEVOPEDIDO(reg.np),&reg.np)
			/* Algo va mal */
			default:
				puts("[cargador] Error grave - Esto no tendria que imprmirise");
				exit(2);
		}
		/* Cerrar el fichero i */
		fclose(entrada);
		DEBUGF("[cargador] Fichero cerrado - %d registros leidos\n",contador);
		/* Asignar habilidosamente a la estructura bd */
		*arboles=arb;
		arboles++;
	}

	/* Cargar los datos de servidores y el valor de crun */
	strcpy(filename,nombre_base);
	strcat(filename,FICHERO_CONSTANTES);
	if ( (entrada=fopen(filename,"r")) == NULL ) {
		perror("");
		fprintf(stderr,"[cargador] No puedo abrir el fichero de constantes: \"%s\"\n",filename);
		exit(1);
	}
	if (fscanf(entrada,"%u %llu",&bd->numServ,&bd->numTran)<2) {
		puts("[cargador] ERROR leyendo constantes");
		exit(1);
	}
	fclose(entrada);
}

