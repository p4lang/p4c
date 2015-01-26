/**************************************************************************//**
 *
 * @file
 * @brief behavioral_sim Configuration Header
 *
 * @addtogroup behavioral_sim-config
 * @{
 *
 *****************************************************************************/
#ifndef __BEHAVIORAL_SIM_CONFIG_H__
#define __BEHAVIORAL_SIM_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef BEHAVIORAL_SIM_INCLUDE_CUSTOM_CONFIG
#include <behavioral_sim_custom_config.h>
#endif

/* <auto.start.cdefs(BEHAVIORAL_SIM_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * BEHAVIORAL_SIM_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef BEHAVIORAL_SIM_CONFIG_INCLUDE_LOGGING
#define BEHAVIORAL_SIM_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * BEHAVIORAL_SIM_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef BEHAVIORAL_SIM_CONFIG_LOG_OPTIONS_DEFAULT
#define BEHAVIORAL_SIM_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * BEHAVIORAL_SIM_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef BEHAVIORAL_SIM_CONFIG_LOG_BITS_DEFAULT
#define BEHAVIORAL_SIM_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * BEHAVIORAL_SIM_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef BEHAVIORAL_SIM_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define BEHAVIORAL_SIM_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB
#define BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB 1
#endif

/**
 * BEHAVIORAL_SIM_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef BEHAVIORAL_SIM_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define BEHAVIORAL_SIM_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS BEHAVIORAL_SIM_CONFIG_PORTING_STDLIB
#endif

/**
 * BEHAVIORAL_SIM_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef BEHAVIORAL_SIM_CONFIG_INCLUDE_UCLI
#define BEHAVIORAL_SIM_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct behavioral_sim_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} behavioral_sim_config_settings_t;

/** Configuration settings table. */
/** behavioral_sim_config_settings table. */
extern behavioral_sim_config_settings_t behavioral_sim_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* behavioral_sim_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int behavioral_sim_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(BEHAVIORAL_SIM_CONFIG_HEADER).header> */

#include "behavioral_sim_porting.h"

#endif /* __BEHAVIORAL_SIM_CONFIG_H__ */
/* @} */
