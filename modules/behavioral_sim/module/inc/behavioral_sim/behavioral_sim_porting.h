/**************************************************************************//**
 *
 * @file
 * @brief behavioral_sim Porting Macros.
 *
 * @addtogroup behavioral_sim-porting
 * @{
 *
 *****************************************************************************/
#ifndef __BEHAVIORAL_SIM_PORTING_H__
#define __BEHAVIORAL_SIM_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if BEHAVIORAL_SIM_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef BEHAVIORAL_SIM_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define BEHAVIORAL_SIM_MALLOC GLOBAL_MALLOC
    #elif BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_SIM_MALLOC malloc
    #else
        #error The macro BEHAVIORAL_SIM_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_SIM_FREE
    #if defined(GLOBAL_FREE)
        #define BEHAVIORAL_SIM_FREE GLOBAL_FREE
    #elif BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_SIM_FREE free
    #else
        #error The macro BEHAVIORAL_SIM_FREE is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_SIM_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define BEHAVIORAL_SIM_MEMSET GLOBAL_MEMSET
    #elif BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_SIM_MEMSET memset
    #else
        #error The macro BEHAVIORAL_SIM_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_SIM_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define BEHAVIORAL_SIM_MEMCPY GLOBAL_MEMCPY
    #elif BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_SIM_MEMCPY memcpy
    #else
        #error The macro BEHAVIORAL_SIM_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_SIM_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define BEHAVIORAL_SIM_STRNCPY GLOBAL_STRNCPY
    #elif BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_SIM_STRNCPY strncpy
    #else
        #error The macro BEHAVIORAL_SIM_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_SIM_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define BEHAVIORAL_SIM_VSNPRINTF GLOBAL_VSNPRINTF
    #elif BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_SIM_VSNPRINTF vsnprintf
    #else
        #error The macro BEHAVIORAL_SIM_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_SIM_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define BEHAVIORAL_SIM_SNPRINTF GLOBAL_SNPRINTF
    #elif BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_SIM_SNPRINTF snprintf
    #else
        #error The macro BEHAVIORAL_SIM_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_SIM_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define BEHAVIORAL_SIM_STRLEN GLOBAL_STRLEN
    #elif BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_SIM_STRLEN strlen
    #else
        #error The macro BEHAVIORAL_SIM_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __BEHAVIORAL_SIM_PORTING_H__ */
/* @} */
