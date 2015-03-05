/**************************************************************************//**
 *
 * @file
 * @brief BMI Configuration Header
 *
 * @addtogroup bmi-config
 * @{
 *
 *****************************************************************************/
#ifndef __BMI_CONFIG_H__
#define __BMI_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef BMI_INCLUDE_CUSTOM_CONFIG
#include <bmi_custom_config.h>
#endif

/* <auto.start.cdefs(BMI_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * BMI_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef BMI_CONFIG_INCLUDE_LOGGING
#define BMI_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * BMI_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef BMI_CONFIG_LOG_OPTIONS_DEFAULT
#define BMI_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * BMI_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef BMI_CONFIG_LOG_BITS_DEFAULT
#define BMI_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * BMI_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef BMI_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define BMI_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * BMI_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef BMI_CONFIG_PORTING_STDLIB
#define BMI_CONFIG_PORTING_STDLIB 1
#endif

/**
 * BMI_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef BMI_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define BMI_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS BMI_CONFIG_PORTING_STDLIB
#endif

/**
 * BMI_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef BMI_CONFIG_INCLUDE_UCLI
#define BMI_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct bmi_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} bmi_config_settings_t;

/** Configuration settings table. */
/** bmi_config_settings table. */
extern bmi_config_settings_t bmi_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* bmi_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int bmi_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(BMI_CONFIG_HEADER).header> */

#include "bmi_porting.h"

#endif /* __BMI_CONFIG_H__ */
/* @} */
