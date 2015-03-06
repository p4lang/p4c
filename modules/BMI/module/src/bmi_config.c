/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <BMI/bmi_config.h>

/* <auto.start.cdefs(BMI_CONFIG_HEADER).source> */
#define __bmi_config_STRINGIFY_NAME(_x) #_x
#define __bmi_config_STRINGIFY_VALUE(_x) __bmi_config_STRINGIFY_NAME(_x)
bmi_config_settings_t bmi_config_settings[] =
{
#ifdef BMI_CONFIG_INCLUDE_LOGGING
    { __bmi_config_STRINGIFY_NAME(BMI_CONFIG_INCLUDE_LOGGING), __bmi_config_STRINGIFY_VALUE(BMI_CONFIG_INCLUDE_LOGGING) },
#else
{ BMI_CONFIG_INCLUDE_LOGGING(__bmi_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BMI_CONFIG_LOG_OPTIONS_DEFAULT
    { __bmi_config_STRINGIFY_NAME(BMI_CONFIG_LOG_OPTIONS_DEFAULT), __bmi_config_STRINGIFY_VALUE(BMI_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ BMI_CONFIG_LOG_OPTIONS_DEFAULT(__bmi_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BMI_CONFIG_LOG_BITS_DEFAULT
    { __bmi_config_STRINGIFY_NAME(BMI_CONFIG_LOG_BITS_DEFAULT), __bmi_config_STRINGIFY_VALUE(BMI_CONFIG_LOG_BITS_DEFAULT) },
#else
{ BMI_CONFIG_LOG_BITS_DEFAULT(__bmi_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BMI_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __bmi_config_STRINGIFY_NAME(BMI_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __bmi_config_STRINGIFY_VALUE(BMI_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ BMI_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__bmi_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BMI_CONFIG_PORTING_STDLIB
    { __bmi_config_STRINGIFY_NAME(BMI_CONFIG_PORTING_STDLIB), __bmi_config_STRINGIFY_VALUE(BMI_CONFIG_PORTING_STDLIB) },
#else
{ BMI_CONFIG_PORTING_STDLIB(__bmi_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BMI_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __bmi_config_STRINGIFY_NAME(BMI_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __bmi_config_STRINGIFY_VALUE(BMI_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ BMI_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__bmi_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BMI_CONFIG_INCLUDE_UCLI
    { __bmi_config_STRINGIFY_NAME(BMI_CONFIG_INCLUDE_UCLI), __bmi_config_STRINGIFY_VALUE(BMI_CONFIG_INCLUDE_UCLI) },
#else
{ BMI_CONFIG_INCLUDE_UCLI(__bmi_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __bmi_config_STRINGIFY_VALUE
#undef __bmi_config_STRINGIFY_NAME

const char*
bmi_config_lookup(const char* setting)
{
    int i;
    for(i = 0; bmi_config_settings[i].name; i++) {
        if(strcmp(bmi_config_settings[i].name, setting)) {
            return bmi_config_settings[i].value;
        }
    }
    return NULL;
}

int
bmi_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; bmi_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", bmi_config_settings[i].name, bmi_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(BMI_CONFIG_HEADER).source> */

