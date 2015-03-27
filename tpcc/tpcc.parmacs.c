#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "cargador.h"
//#define DEBUG
#include "debug.h"

/* Parmacs */
MAIN_ENV

/* Variables globales - compartidas */
uint64_t **estadisticas=NULL;
int *indice=NULL;

/* Para los semaforos y poder reservar su memoria mas tarde */
struct semaforos {
	LOCKDEC(indice);
} *sems=NULL;

/* La barrera necesaria de inicializacion */
struct barreras {
	BARDEC(inicio);
} *bars=NULL;

/* Configuracion general */
#define __ESTADISTICAS__
#define __LIMPIAR__

void servidor(void);
void limpiar(BBDD *);

/* Variables globales - NO compartidas */
BBDD principal;
char nombre_base[512];
char *trans_text[5]={"Nuevo Pedido","Pago","Estado de Pedido","Envio","Nivel de Existencias"};

int main(int argc,char *argv[]) {
	time_t inicio,fin;
	int i,j;

	/* Parmacs */
	MAIN_INITENV()

	/* Parametros de entrada
	 * 1) Nombre del experimento
	 * 2) Directorio de trabajo (por defecto el actual)
	 */
	if (argc==3) snprintf(nombre_base,512,"%s/%s_",argv[2],argv[1]);
	else if (argc==2) {
		strncpy(nombre_base,argv[1],510);
		strcat(nombre_base,"_");
		nombre_base[511]='\0';
	}
	else {
		DEBUGF("Modo de uso:\n"
		       "\t%s <nombre_experimento> [<directorio_base>]\n",argv[0]);
		return;
	}
	DEBUGF("[tpcc] Inicio - Nombre base: \"%s\"\n",nombre_base);
	/* Inicializar la base de datos con la informacion generada */
	DEBUGS("[tpcc] Inicio - Poblando la base de datos, esto llevará un buen rato");
	/* Desactivar la salida durante la carga */
	//int tmp;
	//tmp=dup(1);
	//close(1);
	cargador(&principal,nombre_base);
	//dup2(tmp,1);
	DEBUGF("[tpcc] Numero servidores:%u Numero transacciones:%llu\n",principal.numServ,principal.numTran);

	/* MEMORIA COMPARTIDA */
	/* Preparar memoria compartida para estadísticas */
	estadisticas=(uint64_t **)G_MALLOC(sizeof(uint64_t*)*principal.numServ);
	for(i=0;i<principal.numServ;i++) {
		/* Un vector de 5 posiciones que anota la cantidad de transacciones de un tipo */
		estadisticas[i]=(uint64_t *)G_MALLOC(sizeof(uint64_t)*5)
		bzero(estadisticas[i],sizeof(uint64_t)*5);
	}
	/* Memoria compartida de indice de servidores y semáforo */
	sems=(struct semaforos *)G_MALLOC(sizeof(struct semaforos));
	bars=(struct  barreras *)G_MALLOC(sizeof(struct  barreras));
	indice=(int*)G_MALLOC(sizeof(int));
	/* Inicializacion */
	LOCKINIT(sems->indice);
	BARINIT(bars->inicio);
	*indice=0;
	
	/* Lanzar los servidores */
	for(i=0;i<principal.numServ;i++) CREATE(servidor)

	/* Contabilizar tiempo */
	BARRIER(bars->inicio,principal.numServ+1);
	inicio=time(NULL);
	WAIT_FOR_END()
	fin=time(NULL);
		
	/* Estadisticas */
#ifdef __ESTADISTICAS__
	uint64_t tottrans=0;
	FILE *fstad;
	
	/* Totales */
	for(i=0;i<principal.numServ;i++)
		for(j=0;j<5;j++) tottrans+=estadisticas[i][j];
	/* Por tipo - Usamos el almacen 0 */
	for(i=1;i<principal.numServ;i++) 
		for(j=0;j<5;j++) estadisticas[0][j]+=estadisticas[i][j];
	strcat(nombre_base,"estadisticas.txt");
	if ((fstad=fopen(nombre_base,"w"))==NULL) {
		perror("");
		fprintf(stderr,"[tpcc] No puedo abrir fichero de estadisticas\n");
		exit(1);
	}
	fprintf(fstad,"Estadisticas generadas: %s\n",ctime(&fin));
	fprintf(fstad,"Total transacciones: %llu (%llu programadas)\n",tottrans,principal.numTran);
	for(i=0;i<5;i++) fprintf(fstad,"\t* %s %.4f%% (%llu)\n",
	                         trans_text[i],((float)estadisticas[0][i]*100)/(float)tottrans,
				 estadisticas[0][i]);
	fprintf(fstad,"Transacciones por segundo: %.4f\n",(float)tottrans/(float)(fin-inicio));
	fprintf(fstad,"(%llu transacciones en %u segundos)\n",tottrans,fin-inicio);
	fprintf(fstad,"tpmC - %.4f (transacciones de nuevo pedido por minuto)\n",
	 (float)estadisticas[0][0]/((float)(fin-inicio)/60));
	fclose(fstad);
#endif	

#ifdef __LIMPIAR__
	DEBUGS("[tpcc] Limpiando almacen de datos");
	limpiar(&principal);
	DEBUGS("[tpcc] Limpiando memoria compartida");
	G_FREE(sems);
	G_FREE(bars);
	for(i=0;i<principal.numServ;i++) G_FREE(estadisticas[i])
	G_FREE(estadisticas);
	G_FREE(indice);
#endif

	MAIN_END()
	return 0;
}

/** Limpia los medios de almacenamiento */
void limpiar(BBDD *bd) {
	Arbol **arb=(Arbol **)bd;
	int i;

	for(i=0;i<9;i++,arb++) {
		DEBUGF("[limpiar] Destruyendo medio de almacenamiento numero %d\n",i);
		if (i==5) le_destruir((ListaEnlazada *)*arb);
		else abm_destruir(*arb);
	}
}

void servidor(void) {
	int myindex,len,tipo;
	FILE *fentrada;
	char nombre_entrada[512];
	
	/* INICIALIZACION */
	/* 1) Cojer su indice */
	LOCK(sems->indice)
	myindex=*indice;
	(*indice)++;
	UNLOCK(sems->indice)
	DEBUGF("[%d][Servidor] Indice: %d\n",myindex,myindex);

	/* 2) Fichero de entrada */
	memcpy(nombre_entrada,nombre_base,512);
	strcat(nombre_entrada,"carga");
	len=strlen(nombre_entrada);
	sprintf(nombre_entrada+len,"%d",myindex);
	strcat(nombre_entrada,".txt");
	DEBUGF("[%d][Servidor] Fichero entrada: \"%s\"\n",myindex,nombre_entrada);

	if ((fentrada=fopen(nombre_entrada,"r"))==NULL) {
		perror("");
		fprintf(stderr,"[%d][Servidor] No puedo abrir fichero de entrada\n",myindex);
		exit(1);
	}
	
	/* 3) Barrera de espera */
	BARRIER(bars->inicio,principal.numServ+1)
	DEBUGF("[%d][Servidor] Comenzando (Barrera de inicio superada)\n",myindex);

	/* PROCESO PRINCIPAL */
	while ( fscanf(fentrada,"%d\t",&tipo)>0 ) {
		switch(tipo) {
			case 0: DEBUGF("[%d][Servidor] Transaccion de Nuevo pedido\n",myindex);
				nuevo_pedido(&principal,fentrada);
				break;
			case 1: DEBUGF("[%d][Servidor] Transaccion de Pago\n",myindex);
				pago(&principal,fentrada);
				break;
			case 2: DEBUGF("[%d][Servidor] Transaccion de Estado de pedido\n",myindex);
				estado_pedido(&principal,fentrada);
				break;
			case 3: DEBUGF("[%d][Servidor] Transaccion de Envio\n",myindex);
				envio(&principal,fentrada);
				break;
			case 4: DEBUGF("[%d][Servidor] Transaccion de Nivel de existencias\n",myindex);
				nivel_existencias(&principal,fentrada);
				break;
			default: DEBUGF("[%d][Servidor] Esto no tendria que suceder\n",myindex);
				 DEBUGF("[%d][Servidor] FINALIZANDO\n",myindex);
				 return;
		}
#ifdef __ESTADISTICAS__
		estadisticas[myindex][tipo]++;
#endif
	}
	
	/* SALIDA */
	fclose(fentrada);
	DEBUGF("[%d][Servidor] Trabajo finalizado\n",myindex);
#ifdef __ESTADISTICAS__
	uint64_t tottrans=0;
	for(len=0;len<5;len++) tottrans+=estadisticas[myindex][len];
	DEBUGF("[%d][Servidor] Total transacciones: %llu\n",myindex,tottrans);
	for(len=0;len<5;len++)
	       DEBUGF("\t* %s %.4f%% (%llu)\n",
	              trans_text[len],((float)estadisticas[myindex][len]*100)/(float)tottrans,
		      estadisticas[myindex][len]); 
		       
#endif
}
