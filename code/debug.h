/** \defgroup debug Ayuda al desarrollo
    @{ */
 
/**
 *  \file debug.h
 *  Funcionalidad de ayuda al desarrollo.
 *  Funciones para imprmir informacion en caso de estar activada la opción 
 *  DEBUG. Si no está activada es como si no existieran
 */
#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef DEBUG
	/* Necesitamos printf y puts */
	#include <stdio.h>
	/** Equivale a printf si DEBUG está activado */
	#define DEBUGF(...) printf(__VA_ARGS__)

	/** Equivale a puts si DEBUG está activado */
	#define DEBUGS(...) puts(__VA_ARGS__)
	#warning ¡¡Has activado DEBUG para este modulo!!
#else
	/** Equivale a nada si DEBUG está desactivado */
	#define DEBUGF(...)
	
	/** Equivale a nada si DEBUG está desactivado */
	#define DEBUGS(...)
#endif

#endif
/** @} */
