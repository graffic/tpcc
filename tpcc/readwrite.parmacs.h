#ifndef __READWRITE_H__
#define __READWRITE_H__

MAIN_ENV

/** Estructura que define un sistema de bloqueo lector/escritor */
struct struct_rw {
	/** Cerrojo que actua como cola a la hora de encolar 
	las operaciones de lectura o de escritura que se vayan solicitando. */
        LOCKDEC(cola)
	/** Cerrojo que protege la variable controla el numero de lectores actuales: 
	num_lectores. */
        LOCKDEC(seccion_critica)
	/** Cerrojo utilizado para dar paso a un solo escritor. */
        LOCKDEC(bloqueo_escritores)
	/** Variable que controla el número actual de 
	lectores */
        int num_lectores;
};

/** Tipo de dato para la estructura que define un sistema de bloqueo lector/escritor */
typedef struct struct_rw rw_data;

/* Estado */
rw_data *rw_init(rw_data *);

/* Operaciones */
void     rw_read_ini(rw_data *);
void     rw_read_fin(rw_data *);
void     rw_write_ini(rw_data *);
void     rw_write_fin(rw_data *);

#endif
