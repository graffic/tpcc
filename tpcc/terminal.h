#ifndef __TERMINAL_H__
#define __TERMINAL_H__

/* Para el uint64_t */
#include <stdint.h>

#define DS_TERM_NUEVOPEDIDO      "0\t%u\t%u\t%u\t%u\n"
#define DS_TERM_LINEAPEDIDO      "\t%u\t%u\t%u\n"
#define DS_TERM_PAGO             "1\t%u\t%u\t%u\t%s\t%u\t%u\t%u\t%u\t\n"
#define DS_TERM_ESTADOPEDIDO     "2\t%u\t%u\t%u\t%s\n"
#define DS_TERM_ENVIO            "3\t%u\t%u\t%u\n"
#define DS_TERM_NIVELEXISTENCIAS "4\t%u\t%u\t%u\n"

#define RS_TERM_NUEVOPEDIDO      "%u\t%u\t%u\t%u\n"
#define RS_TERM_LINEAPEDIDO	 "\t%u\t%u\t%u\n"
#define RS_TERM_PAGO		 "%u\t%u\t%u\t%s\t%u\t%u\t%u\t%u\t\n"
#define RS_TERM_ESTADOPEDIDO     "%u\t%u\t%u\t%s\n"
#define RS_TERM_ENVIO		 "%u\t%u\t%u\n"
#define RS_TERM_NIVELEXISTENCIAS "%u\t%u\t%u\n"

#define BARAJA_CARTAS {0,0,0,0,0,0,0,0,0,0,\
		       1,1,1,1,1,1,1,1,1,1,\
		       2,3,4}
#define BARAJA_LEN 23

void terminal_tpcc(FILE *,uint64_t);

#endif
