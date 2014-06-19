/****************************************************************
 * 
 ***************************************************************/

#ifndef __TESTMOD_CONFIG_H__
#define __TESTMOD_CONFIG_H__


#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef TESTMOD_INCLUDE_CUSTOM_CONFIG
#include <testmod_custom_config.h>
#endif

/* <auto.start.cdefs(TESTMOD_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * TESTMOD_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef TESTMOD_CONFIG_INCLUDE_LOGGING
#define TESTMOD_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * TESTMOD_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef TESTMOD_CONFIG_LOG_OPTIONS_DEFAULT
#define TESTMOD_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * TESTMOD_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log options. */


#ifndef TESTMOD_CONFIG_LOG_BITS_DEFAULT
#define TESTMOD_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * TESTMOD_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log options. */


#ifndef TESTMOD_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define TESTMOD_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * TESTMOD_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef TESTMOD_CONFIG_PORTING_STDLIB
#define TESTMOD_CONFIG_PORTING_STDLIB 1
#endif

/**
 * TESTMOD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef TESTMOD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define TESTMOD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS TESTMOD_CONFIG_PORTING_STDLIB
#endif

/**
 * TESTMOD_CONFIG_INCLUDE_UCLI
 *
 * Include Generic UCLI support. */


#ifndef TESTMOD_CONFIG_INCLUDE_UCLI
#define TESTMOD_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** @brief Configuration settings structure. */
typedef struct testmod_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} testmod_config_settings_t;

/** Configuration settings table. */
/** testmod_config_settings table. */
extern testmod_config_settings_t testmod_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* testmod_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int testmod_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(TESTMOD_CONFIG_HEADER).header> */

#include "testmod_porting.h"

#endif /* __TESTMOD_CONFIG_H__ */

