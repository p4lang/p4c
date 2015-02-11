/**************************************************************************//**
 *
 * @file
 * @brief behavioral_utils Porting Macros.
 *
 * @addtogroup behavioral_utils-porting
 * @{
 *
 *****************************************************************************/
#ifndef __BEHAVIORAL_UTILS_PORTING_H__
#define __BEHAVIORAL_UTILS_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if BEHAVIORAL_UTILS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef BEHAVIORAL_UTILS_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define BEHAVIORAL_UTILS_MALLOC GLOBAL_MALLOC
    #elif BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_UTILS_MALLOC malloc
    #else
        #error The macro BEHAVIORAL_UTILS_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_UTILS_FREE
    #if defined(GLOBAL_FREE)
        #define BEHAVIORAL_UTILS_FREE GLOBAL_FREE
    #elif BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_UTILS_FREE free
    #else
        #error The macro BEHAVIORAL_UTILS_FREE is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_UTILS_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define BEHAVIORAL_UTILS_MEMSET GLOBAL_MEMSET
    #elif BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_UTILS_MEMSET memset
    #else
        #error The macro BEHAVIORAL_UTILS_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_UTILS_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define BEHAVIORAL_UTILS_MEMCPY GLOBAL_MEMCPY
    #elif BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_UTILS_MEMCPY memcpy
    #else
        #error The macro BEHAVIORAL_UTILS_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_UTILS_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define BEHAVIORAL_UTILS_STRNCPY GLOBAL_STRNCPY
    #elif BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_UTILS_STRNCPY strncpy
    #else
        #error The macro BEHAVIORAL_UTILS_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_UTILS_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define BEHAVIORAL_UTILS_VSNPRINTF GLOBAL_VSNPRINTF
    #elif BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_UTILS_VSNPRINTF vsnprintf
    #else
        #error The macro BEHAVIORAL_UTILS_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_UTILS_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define BEHAVIORAL_UTILS_SNPRINTF GLOBAL_SNPRINTF
    #elif BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_UTILS_SNPRINTF snprintf
    #else
        #error The macro BEHAVIORAL_UTILS_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef BEHAVIORAL_UTILS_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define BEHAVIORAL_UTILS_STRLEN GLOBAL_STRLEN
    #elif BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB == 1
        #define BEHAVIORAL_UTILS_STRLEN strlen
    #else
        #error The macro BEHAVIORAL_UTILS_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __BEHAVIORAL_UTILS_PORTING_H__ */
/* @} */
