modulos = abm_test.o arbolbmas.o basicgen.o cargador.o comparadores.o \
          generador.o le_test.o listaenlazada.o readwrite.o \
	  tpcc.o transacciones.o terminal.o
objetos_abm_test = abm_test.o arbolbmas.o readwrite.o
objetos_le_test  = le_test.o listaenlazada.o
objetos_generador= generador.o basicgen.o terminal.o
objetos_tpcc = tpcc.o arbolbmas.o cargador.o comparadores.o listaenlazada.o transacciones.o basicgen.o\
		readwrite.o 
programas   = tpcc generador abm_test le_test

# Variables Generales
CFLAGS=-g 
#CFLAGS=-g -DDEBUG 
LDFLAGS=-lpthread
CC=gcc
MACROS=macros/c.m4.posix.mutex

programas: $(programas)


le_test: $(objetos_le_test)
abm_test: $(objetos_abm_test)
generador: $(objetos_generador)
tpcc: $(objetos_tpcc)

# Modulos
abm_test.o: abm_test.parmacs.c arbolbmas.h arbolbmas-priv.h readwrite.h
arbolbmas.o: arbolbmas.parmacs.c debug.h arbolbmas.h arbolbmas-priv.h readwrite.h
basicgen.o: basicgen.c basicgen.h
cargador.o: cargador.c cargador.h generador.h debug.h arbolbmas.h comparadores.h registros.h listaenlazada.h
comparadores.o: comparadores.c registros.h
generador.o: generador.c generador.h basicgen.h registros.h
le_test.o: le_test.c listaenlazada.h
listaenlazada.o: debug.h listaenlazada.parmacs.c
terminal.o: terminal.c terminal.h debug.h
tpcc.o: listaenlazada.h tpcc.parmacs.c cargador.h debug.h readwrite.h
transacciones.o: transacciones.c generador.h cargador.h debug.h basicgen.h registros.h listaenlazada.h arbolbmas.h
readwrite.o: readwrite.parmacs.c readwrite.h


# Ficheros salidos del m4
fuentes_interpretados =\
	arbolbmas.c \
	listaenlazada.h \
	tpcc.c \
	readwrite.c \
	readwrite.h

# Transformacion de parmacs a C
.SUFFIXES: .parmacs.c .parmacs.h
.parmacs.c.c: $(MACROS)
	m4 $(MACROS) $< > $@
.parmacs.h.h: $(MACROS)
	m4 $(MACROS) $< > $@

.PHONY: clean
clean:
	rm -f $(programas) $(modulos) $(fuentes_interpretados)
	rm -f listaenlazada.h readwrite.h
