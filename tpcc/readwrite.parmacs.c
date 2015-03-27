#include "readwrite.h"
//#define DEBUG
#include "debug.h"

void rw_read_ini(rw_data *bloq) {
	DEBUGF("[%lu][rw_read_ini] Esperando en la cola (bloq: %p)\n",GET_PID,bloq);fflush(NULL);
	LOCK(bloq->cola)
	DEBUGF("[%lu][rw_read_ini] Esperando en la seccion critica (bloq: %p)\n",GET_PID,bloq);fflush(NULL);
	LOCK(bloq->seccion_critica)
		bloq->num_lectores++;
		DEBUGF("[%lu][rw_read_ini] Lectores actuales %d (bloq: %p)\n",GET_PID,bloq->num_lectores,bloq);fflush(NULL);
		if (bloq->num_lectores == 1) LOCK(bloq->bloqueo_escritores)
	UNLOCK(bloq->seccion_critica)
	UNLOCK(bloq->cola)
}

void rw_read_fin(rw_data *bloq) {
	DEBUGF("[%lu][rw_read_fin] Esperando en la seccion critica (bloq: %p)\n",GET_PID,bloq);fflush(NULL);
	LOCK(bloq->seccion_critica)
		bloq->num_lectores--;
		DEBUGF("[%lu][rw_read_fin] Lectores actuales %d (bloq: %p)\n",GET_PID,bloq->num_lectores,bloq);fflush(NULL);
		if(bloq->num_lectores == 0) UNLOCK(bloq->bloqueo_escritores)
	UNLOCK(bloq->seccion_critica)
}

void rw_write_ini(rw_data *bloq) {
	DEBUGF("[%lu][rw_write_ini] Esperando en la cola (bloq: %p)\n",GET_PID,bloq);fflush(NULL);
	LOCK(bloq->cola)
	DEBUGF("[%lu][rw_write_ini] Esperando posibilidad de escritura (bloq: %p)\n",GET_PID,bloq);fflush(NULL);
	LOCK(bloq->bloqueo_escritores)
	UNLOCK(bloq->cola)
}
void rw_write_fin(rw_data *bloq) {
	DEBUGF("[%lu][rw_write_fin] Fin de escritura (bloq: %p)\n",GET_PID,bloq);fflush(NULL);
	UNLOCK(bloq->bloqueo_escritores)
}


rw_data *rw_init(rw_data *nuevo) {
	DEBUGF("[%lu][rw_nuevo] Inicializando semaforos (bloq: %p)\n",GET_PID,nuevo);fflush(NULL);
	LOCKINIT(nuevo->cola)
	LOCKINIT(nuevo->seccion_critica)
	LOCKINIT(nuevo->bloqueo_escritores)
	nuevo->num_lectores=0;

	return nuevo;
}

