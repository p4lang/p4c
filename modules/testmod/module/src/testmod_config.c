/****************************************************************
 * 
 ***************************************************************/

#include <testmod/testmod_config.h> 
#include <stdlib.h>
#include <string.h>



/* <auto.start.cdefs(TESTMOD_CONFIG_HEADER).source> */
#define __testmod_config_STRINGIFY_NAME(_x) #_x
#define __testmod_config_STRINGIFY_VALUE(_x) __testmod_config_STRINGIFY_NAME(_x)
testmod_config_settings_t testmod_config_settings[] =
{
#ifdef TESTMOD_CONFIG_INCLUDE_LOGGING
    { __testmod_config_STRINGIFY_NAME(TESTMOD_CONFIG_INCLUDE_LOGGING), __testmod_config_STRINGIFY_VALUE(TESTMOD_CONFIG_INCLUDE_LOGGING) },
#else
{ TESTMOD_CONFIG_INCLUDE_LOGGING(__testmod_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef TESTMOD_CONFIG_LOG_OPTIONS_DEFAULT
    { __testmod_config_STRINGIFY_NAME(TESTMOD_CONFIG_LOG_OPTIONS_DEFAULT), __testmod_config_STRINGIFY_VALUE(TESTMOD_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ TESTMOD_CONFIG_LOG_OPTIONS_DEFAULT(__testmod_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef TESTMOD_CONFIG_LOG_BITS_DEFAULT
    { __testmod_config_STRINGIFY_NAME(TESTMOD_CONFIG_LOG_BITS_DEFAULT), __testmod_config_STRINGIFY_VALUE(TESTMOD_CONFIG_LOG_BITS_DEFAULT) },
#else
{ TESTMOD_CONFIG_LOG_BITS_DEFAULT(__testmod_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef TESTMOD_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __testmod_config_STRINGIFY_NAME(TESTMOD_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __testmod_config_STRINGIFY_VALUE(TESTMOD_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ TESTMOD_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__testmod_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef TESTMOD_CONFIG_PORTING_STDLIB
    { __testmod_config_STRINGIFY_NAME(TESTMOD_CONFIG_PORTING_STDLIB), __testmod_config_STRINGIFY_VALUE(TESTMOD_CONFIG_PORTING_STDLIB) },
#else
{ TESTMOD_CONFIG_PORTING_STDLIB(__testmod_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef TESTMOD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __testmod_config_STRINGIFY_NAME(TESTMOD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __testmod_config_STRINGIFY_VALUE(TESTMOD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ TESTMOD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__testmod_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef TESTMOD_CONFIG_INCLUDE_UCLI
    { __testmod_config_STRINGIFY_NAME(TESTMOD_CONFIG_INCLUDE_UCLI), __testmod_config_STRINGIFY_VALUE(TESTMOD_CONFIG_INCLUDE_UCLI) },
#else
{ TESTMOD_CONFIG_INCLUDE_UCLI(__testmod_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __testmod_config_STRINGIFY_VALUE
#undef __testmod_config_STRINGIFY_NAME

const char*
testmod_config_lookup(const char* setting)
{
    int i;
    for(i = 0; testmod_config_settings[i].name; i++) {
        if(strcmp(testmod_config_settings[i].name, setting)) {
            return testmod_config_settings[i].value;
        }
    }
    return NULL;
}

int
testmod_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; testmod_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", testmod_config_settings[i].name, testmod_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(TESTMOD_CONFIG_HEADER).source> */



