/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <behavioral_utils/behavioral_utils_config.h>

/* <auto.start.cdefs(BEHAVIORAL_UTILS_CONFIG_HEADER).source> */
#define __behavioral_utils_config_STRINGIFY_NAME(_x) #_x
#define __behavioral_utils_config_STRINGIFY_VALUE(_x) __behavioral_utils_config_STRINGIFY_NAME(_x)
behavioral_utils_config_settings_t behavioral_utils_config_settings[] =
{
#ifdef BEHAVIORAL_UTILS_CONFIG_INCLUDE_LOGGING
    { __behavioral_utils_config_STRINGIFY_NAME(BEHAVIORAL_UTILS_CONFIG_INCLUDE_LOGGING), __behavioral_utils_config_STRINGIFY_VALUE(BEHAVIORAL_UTILS_CONFIG_INCLUDE_LOGGING) },
#else
{ BEHAVIORAL_UTILS_CONFIG_INCLUDE_LOGGING(__behavioral_utils_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BEHAVIORAL_UTILS_CONFIG_LOG_OPTIONS_DEFAULT
    { __behavioral_utils_config_STRINGIFY_NAME(BEHAVIORAL_UTILS_CONFIG_LOG_OPTIONS_DEFAULT), __behavioral_utils_config_STRINGIFY_VALUE(BEHAVIORAL_UTILS_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ BEHAVIORAL_UTILS_CONFIG_LOG_OPTIONS_DEFAULT(__behavioral_utils_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BEHAVIORAL_UTILS_CONFIG_LOG_BITS_DEFAULT
    { __behavioral_utils_config_STRINGIFY_NAME(BEHAVIORAL_UTILS_CONFIG_LOG_BITS_DEFAULT), __behavioral_utils_config_STRINGIFY_VALUE(BEHAVIORAL_UTILS_CONFIG_LOG_BITS_DEFAULT) },
#else
{ BEHAVIORAL_UTILS_CONFIG_LOG_BITS_DEFAULT(__behavioral_utils_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BEHAVIORAL_UTILS_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __behavioral_utils_config_STRINGIFY_NAME(BEHAVIORAL_UTILS_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __behavioral_utils_config_STRINGIFY_VALUE(BEHAVIORAL_UTILS_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ BEHAVIORAL_UTILS_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__behavioral_utils_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB
    { __behavioral_utils_config_STRINGIFY_NAME(BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB), __behavioral_utils_config_STRINGIFY_VALUE(BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB) },
#else
{ BEHAVIORAL_UTILS_CONFIG_PORTING_STDLIB(__behavioral_utils_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BEHAVIORAL_UTILS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __behavioral_utils_config_STRINGIFY_NAME(BEHAVIORAL_UTILS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __behavioral_utils_config_STRINGIFY_VALUE(BEHAVIORAL_UTILS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ BEHAVIORAL_UTILS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__behavioral_utils_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef BEHAVIORAL_UTILS_CONFIG_INCLUDE_UCLI
    { __behavioral_utils_config_STRINGIFY_NAME(BEHAVIORAL_UTILS_CONFIG_INCLUDE_UCLI), __behavioral_utils_config_STRINGIFY_VALUE(BEHAVIORAL_UTILS_CONFIG_INCLUDE_UCLI) },
#else
{ BEHAVIORAL_UTILS_CONFIG_INCLUDE_UCLI(__behavioral_utils_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __behavioral_utils_config_STRINGIFY_VALUE
#undef __behavioral_utils_config_STRINGIFY_NAME

const char*
behavioral_utils_config_lookup(const char* setting)
{
    int i;
    for(i = 0; behavioral_utils_config_settings[i].name; i++) {
        if(strcmp(behavioral_utils_config_settings[i].name, setting)) {
            return behavioral_utils_config_settings[i].value;
        }
    }
    return NULL;
}

int
behavioral_utils_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; behavioral_utils_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", behavioral_utils_config_settings[i].name, behavioral_utils_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(BEHAVIORAL_UTILS_CONFIG_HEADER).source> */

