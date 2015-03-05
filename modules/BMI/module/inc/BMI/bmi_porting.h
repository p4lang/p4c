/**************************************************************************//**
 *
 * @file
 * @brief BMI Porting Macros.
 *
 * @addtogroup bmi-porting
 * @{
 *
 *****************************************************************************/
#ifndef __BMI_PORTING_H__
#define __BMI_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if BMI_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef BMI_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define BMI_MALLOC GLOBAL_MALLOC
    #elif BMI_CONFIG_PORTING_STDLIB == 1
        #define BMI_MALLOC malloc
    #else
        #error The macro BMI_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef BMI_FREE
    #if defined(GLOBAL_FREE)
        #define BMI_FREE GLOBAL_FREE
    #elif BMI_CONFIG_PORTING_STDLIB == 1
        #define BMI_FREE free
    #else
        #error The macro BMI_FREE is required but cannot be defined.
    #endif
#endif

#ifndef BMI_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define BMI_MEMSET GLOBAL_MEMSET
    #elif BMI_CONFIG_PORTING_STDLIB == 1
        #define BMI_MEMSET memset
    #else
        #error The macro BMI_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef BMI_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define BMI_MEMCPY GLOBAL_MEMCPY
    #elif BMI_CONFIG_PORTING_STDLIB == 1
        #define BMI_MEMCPY memcpy
    #else
        #error The macro BMI_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef BMI_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define BMI_STRNCPY GLOBAL_STRNCPY
    #elif BMI_CONFIG_PORTING_STDLIB == 1
        #define BMI_STRNCPY strncpy
    #else
        #error The macro BMI_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef BMI_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define BMI_VSNPRINTF GLOBAL_VSNPRINTF
    #elif BMI_CONFIG_PORTING_STDLIB == 1
        #define BMI_VSNPRINTF vsnprintf
    #else
        #error The macro BMI_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef BMI_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define BMI_SNPRINTF GLOBAL_SNPRINTF
    #elif BMI_CONFIG_PORTING_STDLIB == 1
        #define BMI_SNPRINTF snprintf
    #else
        #error The macro BMI_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef BMI_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define BMI_STRLEN GLOBAL_STRLEN
    #elif BMI_CONFIG_PORTING_STDLIB == 1
        #define BMI_STRLEN strlen
    #else
        #error The macro BMI_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __BMI_PORTING_H__ */
/* @} */
