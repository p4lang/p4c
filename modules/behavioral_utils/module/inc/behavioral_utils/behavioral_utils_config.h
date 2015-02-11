/**************************************************************************//**
 *
 * @file
 * @brief behavioral_utils Configuration Header
 *
 * @addtogroup behavioral_utils-config
 * @{
 *
 *****************************************************************************/
#ifndef __BEHAVIORAL_UTILS_CONFIG_H__
#define __BEHAVIORAL_UTILS_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef BEHAVIORAL_UTILS_INCLUDE_CUSTOM_CONFIG
#include <behavioral_utils_custom_config.h>
#endif

/* <auto.start.cdefs(BEHAVIORAL_UTILS_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * BEHAVIORAL_UTILS_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef BEHAVIORAL_UTILS_CONFIG_INCLUDE_LOGGING
#define BEHAVIORAL_UTILS_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * BEHAVIORAL_UTILS_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef BEHAVIORAL_UTILS_CONFIG_LOG_OPTIONS_DEFAULT
#define BEHAVIORAL_UTILS_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * BEHAVIORAL_UTILS_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef BEHAVIORAL_UTILS_CONFIG_LOG_BITS_DEFAULT
#define BEHAVIORAL_UTILS_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * BEHAVIORAL_UTILS_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef BEHAVIORAL_UTILS_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define BEHAVIORAL_UTILS_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB
#define BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB 1
#endif

/**
 * BEHAVIORAL_UTILS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef BEHAVIORAL_UTILS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define BEHAVIORAL_UTILS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB
#endif

/**
 * BEHAVIORAL_UTILS_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef BEHAVIORAL_UTILS_CONFIG_INCLUDE_UCLI
#define BEHAVIORAL_UTILS_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct behavioral_utils_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} behavioral_utils_config_settings_t;

/** Configuration settings table. */
/** behavioral_utils_config_settings table. */
extern behavioral_utils_config_settings_t behavioral_utils_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* behavioral_utils_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int behavioral_utils_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(BEHAVIORAL_UTILS_CONFIG_HEADER).header> */

#include "behavioral_utils_porting.h"

#endif /* __BEHAVIORAL_UTILS_CONFIG_H__ */
/* @} */
