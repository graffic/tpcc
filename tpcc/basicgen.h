/** Generadores básicos de elementos aleatorios.
    Útiles para el poblado de la base de datos y la generacion de 
    nuevas transacciones
*/
#ifndef __BASICGEN_H__
#define __BASICGEN_H__
#include <sys/types.h>
#include <stdint.h>

void      gen_a_string(char *,int,int);
void      gen_n_string(char *,uint32_t,uint32_t);
void      gen_zip     (char *);
uint32_t gen_number  (uint32_t,uint32_t);
void      gen_last    (char *,int);

/** Genera un numero aleatorio con una distribucion no uniforme.
    Necesita de la variable C (indicada como CValue), para funcionar correctamente.
*/
#define gen_NURand(A,x,y) ((((gen_number(0,A) | gen_number(x,y))+CValue) % (y-x+1))+x)

#define MAPA_NUMEROS    "0123456789"
#define MAPA_CARACTERES "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_."
#define MAPACAR_LEN     66
#define MAPANUM_LEN     10


#endif /* __BASICGEN_H__ */
