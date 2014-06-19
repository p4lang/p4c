/****************************************************************
 * 
 ***************************************************************/
#ifndef __TESTMOD_PORTING_H__
#define __TESTMOD_PORTING_H__


#include <testmod/testmod_config.h>


/* <auto.start.portingmacro(ALL).define> */
#if TESTMOD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef TESTMOD_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define TESTMOD_MEMSET GLOBAL_MEMSET
    #elif TESTMOD_CONFIG_PORTING_STDLIB == 1
        #define TESTMOD_MEMSET memset
    #else
        #error The macro TESTMOD_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef TESTMOD_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define TESTMOD_MALLOC GLOBAL_MALLOC
    #elif TESTMOD_CONFIG_PORTING_STDLIB == 1
        #define TESTMOD_MALLOC malloc
    #else
        #error The macro TESTMOD_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef TESTMOD_FREE
    #if defined(GLOBAL_FREE)
        #define TESTMOD_FREE GLOBAL_FREE
    #elif TESTMOD_CONFIG_PORTING_STDLIB == 1
        #define TESTMOD_FREE free
    #else
        #error The macro TESTMOD_FREE is required but cannot be defined.
    #endif
#endif

#ifndef TESTMOD_MEMCMP
    #if defined(GLOBAL_MEMCMP)
        #define TESTMOD_MEMCMP GLOBAL_MEMCMP
    #elif TESTMOD_CONFIG_PORTING_STDLIB == 1
        #define TESTMOD_MEMCMP memcmp
    #else
        #error The macro TESTMOD_MEMCMP is required but cannot be defined.
    #endif
#endif

#ifndef TESTMOD_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define TESTMOD_MEMCPY GLOBAL_MEMCPY
    #elif TESTMOD_CONFIG_PORTING_STDLIB == 1
        #define TESTMOD_MEMCPY memcpy
    #else
        #error The macro TESTMOD_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef TESTMOD_MEMMOVE
    #if defined(GLOBAL_MEMMOVE)
        #define TESTMOD_MEMMOVE GLOBAL_MEMMOVE
    #elif TESTMOD_CONFIG_PORTING_STDLIB == 1
        #define TESTMOD_MEMMOVE memmove
    #else
        #error The macro TESTMOD_MEMMOVE is required but cannot be defined.
    #endif
#endif

#ifndef TESTMOD_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define TESTMOD_STRNCPY GLOBAL_STRNCPY
    #elif TESTMOD_CONFIG_PORTING_STDLIB == 1
        #define TESTMOD_STRNCPY strncpy
    #else
        #error The macro TESTMOD_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef TESTMOD_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define TESTMOD_STRLEN GLOBAL_STRLEN
    #elif TESTMOD_CONFIG_PORTING_STDLIB == 1
        #define TESTMOD_STRLEN strlen
    #else
        #error The macro TESTMOD_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */

#endif /* __TESTMOD_PORTING_H__ */

