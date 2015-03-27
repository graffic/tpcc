#include <stdio.h>
#include <string.h>
#include <time.h>
#include "arbolbmas.h"
#include "arbolbmas-priv.h"

/* Parmacs */
MAIN_ENV();

#define RP_KEYSIZE sizeof(struct {int num;long cuad;})

struct registroPrueba {
	/* Clave */
	int num;
	long cuad;
	/* Datos */
	long cubo;
	char ascii[15];
};

typedef struct registroPrueba rPrueba;

void modifica(rPrueba *busqueda,rPrueba *original) {
	original->cubo=busqueda->cubo/9;
}

int encajaPares(rPrueba *a,void *b) {
	return (a->num%2)?0:1;
}
int multiplode(rPrueba *a,void *b) {
	int num=(int)b;
	return (a->num%num)?0:1;
}

int compara2Prueba(rPrueba *a,rPrueba *b) {
	// printf("[compara2Pueba]Comparando (%d,%d) con (%d,%d) devuelve ",a->num,a->cuad,b->num,b->cuad);
	if(a->num < b->num) {
		// puts("-1");
		return -1;
	}
	if(a->num > b->num)   {
		// puts("1");
		return 1;
	}
	if(a->cuad < b->cuad) {
		// puts("-1");
		return -1;
	}
	if(a->cuad > b->cuad) {
		// puts("1");
		return 1;
	}
	// puts("0");
	return 0;
}

void rellena(rPrueba *rp,int i) {
	rp->num=i;
	rp->cuad=i*i;
	rp->cubo=i*i*i;
	snprintf(rp->ascii,15,"%d",i);
}

void dump_node(ABMNodo *act,int nivel) {
        char nivelador[100];
	rPrueba *rp;
        int i;
        
	MAIN_INITENV();
	
        /* construir nivelador */
        for (i=0;i<nivel;i++) nivelador[i]='\t';
        nivelador[i]=0;
        printf("%s[Nodo %p] %s ocupados:%d %s:%p\n",nivelador,act,(act->atributos & NODE_ATRIB_INNER)?"INTERNO":"HOJA",
                        act->ocupados,(act->atributos & NODE_ATRIB_INNER)?"padre":"siguiente",act->link);
        if (act->atributos & NODE_ATRIB_INNER) {
                for(i=0;i<act->ocupados;i++) {
                        printf("%s*Enlace:%d apunta:%p\n",nivelador,i,act->enlaces.interno[i]);
                        dump_node(act->enlaces.interno[i],nivel+1);
			rp=getNodeKey(act,i);
			printf("%sClave[%d,%ld](%p)\n",nivelador,rp->num,rp->cuad,rp);	
                }
                printf("%s*Enlace:%d apunta:%p\n",nivelador,i,act->enlaces.interno[i]);
                dump_node(act->enlaces.interno[i],nivel+1);                     
        }
        else {
                for(i=0;i<act->ocupados;i++) {
			rp=getNodeKey(act,i);
                        printf("%sClave[%d,%ld](%p) ",nivelador,rp->num,rp->cuad,rp);
			rp=(rPrueba*)getLeafLink(act,i);
			printf("Datos(%p)(%d,%ld,%ld,\"%s\")\n",getLeafLink(act,i),rp->num,rp->cuad,rp->cubo,rp->ascii);
                }
        }

	MAIN_END();
}

#define dumpReg(rp) printf("dumpReg [%d %d](%d \"%s\")\n",rp.num,rp.cuad,rp.cubo,rp.ascii)
#define MAX       100
#define NORMAL    "\033[22;39m"
#define ROJO      "\033[22;31m"
#define VERDE     "\033[22;32m"
#define AMARILLO  "\033[22;33m"
void autocheck(Arbol *,int);

void borrado(Arbol *arb) {
	int pos[MAX],i,a,b,tmp;
	rPrueba rp;

	for(i=0;i<MAX;i++) pos[i]=i;

	/* barajar */
	srandom(time(NULL));
	for(i=0;i<MAX;i++) {
		a=random()%MAX;
		b=random()%MAX;
		tmp=pos[a];
		pos[a]=pos[b];
		pos[b]=tmp;
	}

	/* borrado */
	for(i=0;i<MAX;i++) {
		rp.num=pos[i];
		rp.cuad=pos[i]*pos[i];
		printf("-----> Borrando %d\n",pos[i]);
		abm_borrar(arb,&rp);
		autocheck(arb,MAX-i-1);
		//puts("=====> Volcando"),dump_node(arb->raiz,0);
	}
	
}

void listCheck(Arbol *arb) {
	ABMNodo *act;
	int i=0,j;
	
	act=arb->raiz;
	
	do {
		for(j=0;j<act->ocupados;j++) {
			
		}
	} while(act!=NULL);
}

void fill(Arbol *arb) {
	rPrueba rp;
	int paso,i,j,contador=0;;

	srandom(time(NULL));
	paso=random()%(MAX/2);
	for(i=0;i<paso;i++)
		for(j=i;j<MAX;j+=paso) {
			rellena(&rp,j);
			printf("%s====> Insertando %d %d%s\n",VERDE,rp.num,rp.cuad,NORMAL);
			abm_insertar(arb,&rp);
			contador++;
			autocheck(arb,contador);
			//puts("=====> Volcando"),dump_node(arb->raiz,0);
		}
}

void autocheck_rec(ABMNodo *);
void autocheck(Arbol *arb,int cuantos) {
	ABMNodo *tmp,*raiz;
	cmpFunc comparar=getCmpFunc(arb);
	char lastKey[arb->keysize];
	int flag=0,i,contador=0;;
	
	puts("[autocheck] Comprobando nodo raiz");
	raiz=getRoot(arb);
	if (raiz->link != NULL) puts("ERROR Nodo raiz tiene padre");
	/* Comprobacion de lista enlazada */
	puts("[autocheck] Comprobando lista enlazada");
	tmp=getFirstNode(arb);
	do {
		/* Dado un nodo comprobamos que la clave siguiente sea superior a la anterior */
		for(i=0;i<getNodeSize(tmp);i++) {
			//printf("- Nodo %p posicion %d (%d)",tmp,i,getNodeSize(tmp));
			if(!flag) flag=1;//puts("");
			else {
				if(comparar(lastKey,getNodeKey(tmp,i)) != -1) puts("ERROR de precedencia");
				//printf(" %d > %d\n",((rPrueba*)(getNodeKey(tmp,i)))->num,*(int*)lastKey);
			}
			memcpy(lastKey,getNodeKey(tmp,i),arb->keysize);
		}
		contador+=i;
		tmp=tmp->link;
	}while(tmp!=NULL);
	if (cuantos!=0 && contador!=cuantos) 
		printf("ERROR Elementos contabilizados %d, nodos esperados %d\n",contador,cuantos);
	puts("[autocheck] Comprobando enlaces");
	autocheck_rec(raiz);
}

void autocheck_rec(ABMNodo *actual) {
	int i;

	/* Comprobar enlaces */
	if(nodo_interno(actual)) { 
		//printf("[autocheck] Nodo interno %p (%d), comprobando enlaces\n",actual,getNodeSize(actual));
		if (nodo_interno(getInnerLink(actual,0))) {
			//puts("* Nodo interno con mas nodos internos a su cargo");
			for(i=0;i<=getNodeSize(actual);i++)
				if (getInnerLink(actual,i)->link != actual) 
					printf("ERROR Nodo padre incorrecto (%p)\n",getInnerLink(actual,i));
		}
		else 
			for(i=1;i<=getNodeSize(actual);i++)
				if (getInnerLink(actual,i) != getInnerLink(actual,i-1)->link ) 
					printf("ERROR Nodo siguiente: de %p es %p debiera ser %p\n"
					,getInnerLink(actual,i-1),getInnerLink(actual,i-1)->link,getInnerLink(actual,i));;
		for(i=0;i<=getNodeSize(actual);i++) {
			//printf("- Descendiendo del nodo %p al nodo %p\n",actual,getInnerLink(actual,i));
			autocheck_rec(getInnerLink(actual,i));
		}
	}
}

int main(int argc, char **argv) {
	Arbol   *arb;
	rPrueba rp;	
	int i,j;
	iterador it;
	/* trabajo a partir de un vector */
	//int vi[]={};
	//int vo[]={};
	
	printf("Tamaño de la clave: %d, tamaño del registro; %d regAddr:%p cmpAddr:%p\n",
		RP_KEYSIZE,sizeof(rPrueba),&rp,&compara2Prueba);

	/* Creamos el arbol */
	arb=abm_nuevo(sizeof(rPrueba),RP_KEYSIZE,(cmpFunc)&compara2Prueba);

	/* Iterador */
	abm_it_init(&it,arb,(itFunc)&encajaPares,NULL);
	puts("=====> Iterando con el arbol vacio");
	while (abm_iterar(&it,&rp)!=ABM_FIN) 
		printf("Iterando: %d %d %d %s\n",rp.num,rp.cuad,rp.cubo,rp.ascii);
	abm_it_fin(&it);
	
	/* duplicado */
	rellena(&rp,0),abm_insertar(arb,&rp);

	/* rellenar */
	fill(arb);
	/* Insercion a partir de un vector */
	//for (i=0;i<MAX;i++) {
	//	rellena(&rp,vi[i]);
	//	printf("=========> Insertar %d %d\n",rp.num,rp.cuad);
	//	abm_insertar(arb,&rp);
	//	autocheck(arb,NULL);
	//}
	
	/* Buscar */
	rp.num=5,rp.cuad=25,rp.cubo=0,rp.ascii[0]='\0';
	if (abm_buscar(arb,&rp)==ABM_OK) printf("Buscando 5,25 -> %d %d %d %s\n",rp.num,rp.cuad,rp.cubo,rp.ascii);
	else puts("ERROR Buscando");

	/* Modificar */
	rp.num=7,rp.cuad=49,strcpy(rp.ascii,"Te cagas");
	rp.cubo=111;
	abm_modificar(arb,&rp,NULL);
	rp.cubo=999;
	abm_modificar(arb,&rp,(doFunc)modifica);

	/* Iterador */
	abm_it_init(&it,arb,(itFunc)&encajaPares,NULL);
	puts("=====> Iterando buscando pares");
	while (abm_iterar(&it,&rp)!=ABM_FIN) 
		printf("Iterando: %d %d %d %s\n",rp.num,rp.cuad,rp.cubo,rp.ascii);
	abm_it_fin(&it);

	/* Iterador chachi */
	abm_it_init(&it,arb,(itFunc)&multiplode,(void *)7);
	puts("=====> Iterando buscando multiplos de 7");
	while (abm_iterar(&it,&rp)!=ABM_FIN) 
		printf("Iterando: %d %d %d %s\n",rp.num,rp.cuad,rp.cubo,rp.ascii);
	abm_it_fin(&it);

	puts("=====> Volcando"),dump_node(arb->raiz,0);
	/* Borrado */
	borrado(arb);
	//for(i=0;i<MAX;i++) {
	//	rellena(&rp,vo[i]);
	//	printf("%s====> Borrando %d %d%s\n",ROJO,rp.num,rp.cuad,NORMAL);
	//	abm_borrar(arb,&rp);
	//	autocheck(arb,NULL);
	//	puts(AMARILLO);	
	//	puts("=====> Volcando"),dump_node(arb->raiz,0);
	//	puts(NORMAL);
	//}

	puts("====> Destruir");
	abm_destruir(arb);
	return 0;
}
