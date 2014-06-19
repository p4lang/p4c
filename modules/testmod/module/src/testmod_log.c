/****************************************************************
 * 
 ***************************************************************/

#include <testmod/testmod_config.h> 
#include "testmod_int.h" 
#include "testmod_log.h"

/* <auto.start.enum(testmod_log_flag).source> */
aim_map_si_t testmod_log_flag_map[] =
{
    { "packet_trace", TESTMOD_LOG_FLAG_PACKET_TRACE },
    { "packet_trace_verbose", TESTMOD_LOG_FLAG_PACKET_TRACE_VERBOSE },
    { "entry_trace", TESTMOD_LOG_FLAG_ENTRY_TRACE },
    { NULL, 0 }
};

aim_map_si_t testmod_log_flag_desc_map[] =
{
    { "None", TESTMOD_LOG_FLAG_PACKET_TRACE },
    { "None", TESTMOD_LOG_FLAG_PACKET_TRACE_VERBOSE },
    { "None", TESTMOD_LOG_FLAG_ENTRY_TRACE },
    { NULL, 0 }
};

const char*
testmod_log_flag_name(testmod_log_flag_t e)
{
    const char* name;
    if(aim_map_si_i(&name, e, testmod_log_flag_map, 0)) {
        return name;
    }
    else {
        return "-invalid value for enum type 'testmod_log_flag'";
    }
}

int
testmod_log_flag_value(const char* str, testmod_log_flag_t* e, int substr)
{
    int i;
    AIM_REFERENCE(substr);
    if(aim_map_si_s(&i, str, testmod_log_flag_map, 0)) {
        /* Enum Found */
        *e = i;
        return 0;
    }
    else {
        return -1;
    }
}

const char*
testmod_log_flag_desc(testmod_log_flag_t e)
{
    const char* name;
    if(aim_map_si_i(&name, e, testmod_log_flag_desc_map, 0)) {
        return name;
    }
    else {
        return "-invalid value for enum type 'testmod_log_flag'";
    }
}

int
testmod_log_flag_valid(testmod_log_flag_t e)
{
    return aim_map_si_i(NULL, e, testmod_log_flag_map, 0) ? 1 : 0;
}

/* <auto.end.enum(testmod_log_flag).source> */

/*
 * testmod log struct
 */

AIM_LOG_STRUCT_DEFINE(
                      TESTMOD_CONFIG_LOG_OPTIONS_DEFAULT,
                      TESTMOD_CONFIG_LOG_BITS_DEFAULT,
                      testmod_log_flag_map, /* Custom log map */
                      TESTMOD_CONFIG_LOG_CUSTOM_BITS_DEFAULT
                      );
