#include <stdlib.h>
#include <string.h>
#include "basicgen.h"


/** Genera un apellido segun las reglas del benchmark TPC-C.
    Los apellidos, segun el punto 4.3.2.3 de la especificacion TPC-C, se
    crean a partir de un numero de 3 digitos, donde cada digito equivale 
    a un vocablo. Los 3 vocablos concatenados forman el apellido.
*/
void gen_last(char *last,int num) {
	char *partes[]={"BAR","OUGHT","ABLE","PRI","PRES","ESE","ANTI","CALLY","ATION","EING"};
	int len[]={3,5,4,3,4,3,4,5,5,4};
	int i,pos=0,tmp,div=100;
	
	/* Hay 3 dígitos */
	for(i=0;i<3;i++) {
		tmp=(num-num%div); /* Sacamos el numero menos el resto */
		num-=tmp;          /* Dejamos en num solo el resto */
		tmp/=div;          /* Calculamos la division exacta */
		memcpy(last+pos,partes[tmp],len[tmp]);
		pos+=len[tmp];
		div/=10;
	}
	last[pos]='\0';
}

/** Genera una cadena de caracteres aleatorios.
    Dado un numero mínimo de caracteres y un número máximo, se crea una
    cadena con caracteres aleatorios entre esas dos posiciones. Aunque tiene
    ciertas restricciones.
    - No empieza las cadenas con espacios.
    - No termina las cadenas con espacios.
*/    
void gen_a_string(char *destino,int a, int b) {
	char caracteres[]=MAPA_CARACTERES;
	int pos,len;
	
	/* Calcular la longitud */
	len=a+(random()%(b-a+1));
	
	for(pos=0;pos<len;pos++) 
		destino[pos]=caracteres[random()%MAPACAR_LEN];	
	
	/* No hay espacios al principio o al final (util para la lectura) */
	if (destino[0]==' ') destino[0]='Ñ';
	if (destino[len-1]==' ') destino[len-1]='ñ';
	
	/* Finalizar la cadena */
	destino[pos]=0;
}

/* Genera una cadena de digutos aleatorios.
   Con un minimo y un máximo de longitud, genera una cadena de dígitos 
   decimales aleatoriamente.
*/
void gen_n_string(char *destino,uint32_t a,uint32_t b) {
	char nummap[]=MAPA_NUMEROS;
	int pos,len;
	
	/* Calcular la longitud */
	len=a+random()%(b-a+1);
	
	/* Generar los numeros aleatorios */
	for(pos=0;pos<len;pos++) 
		destino[pos]=nummap[random()%MAPANUM_LEN];
	
	/* Finalizar la cadena */
	destino[pos]=0;
}

/* Genera un código postal.
   Segun el apartado 4.3.2.7 de la especificacion TPC-C, un código postal se
   crea concatenando a una cadena de 4 digitos aleatoria, la cadena "1111".
*/
void gen_zip (char *destino) {
	/* El código postal se compone de:
	   - cuatro numeros aleatorios */
	gen_n_string(destino,4,4);
	/* - cuatro unos */
	memcpy(destino+4,"1111",5);
}

/** Genera un entero aleatorio entre 2 valores
*/
uint32_t gen_number(uint32_t a,uint32_t b) {
	if (a!=b) return a+(random()%(b-a+1));
	/* Para evitar divisiones por cero */
	else return a;
}

