/****************************************************************
 * 
 ***************************************************************/

#ifndef __TESTMOD_LOG_H__
#define __TESTMOD_LOG_H__

#include <testmod/testmod_config.h>

#define AIM_LOG_MODULE_NAME testmod
#include <AIM/aim_log.h>


/**
 * Custom TESTMOD log types. 
 */

/* <auto.start.enum(testmod_log_flag).header> */
/** testmod_log_flag */
typedef enum testmod_log_flag_e {
    TESTMOD_LOG_FLAG_PACKET_TRACE,
    TESTMOD_LOG_FLAG_PACKET_TRACE_VERBOSE,
    TESTMOD_LOG_FLAG_ENTRY_TRACE,
} testmod_log_flag_t;

/** Enum names. */
const char* testmod_log_flag_name(testmod_log_flag_t e);

/** Enum values. */
int testmod_log_flag_value(const char* str, testmod_log_flag_t* e, int substr);

/** Enum descriptions. */
const char* testmod_log_flag_desc(testmod_log_flag_t e);

/** Enum validator. */
int testmod_log_flag_valid(testmod_log_flag_t e);

/** validator */
#define TESTMOD_LOG_FLAG_VALID(_e) \
    (testmod_log_flag_valid((_e)))

/** testmod_log_flag_map table. */
extern aim_map_si_t testmod_log_flag_map[];
/** testmod_log_flag_desc_map table. */
extern aim_map_si_t testmod_log_flag_desc_map[];
/* <auto.end.enum(testmod_log_flag).header> */

/* <auto.start.aim_custom_log_macro(ALL).header> */

/******************************************************************************
 *
 * Custom Module Log Macros
 *
 *****************************************************************************/

/** Log a module-level packet_trace */
#define TESTMOD_LOG_MOD_PACKET_TRACE(...) \
    AIM_LOG_MOD_CUSTOM(TESTMOD_LOG_FLAG_PACKET_TRACE, "PACKET_TRACE", __VA_ARGS__)
/** Log a module-level packet_trace with ratelimiting */
#define TESTMOD_LOG_MOD_RL_PACKET_TRACE(_rl, _time, ...)           \
    AIM_LOG_MOD_RL_CUSTOM(TESTMOD_LOG_FLAG_PACKET_TRACE, "PACKET_TRACE", _rl, _time, __VA_ARGS__)

/** Log a module-level packet_trace_verbose */
#define TESTMOD_LOG_MOD_PACKET_TRACE_VERBOSE(...) \
    AIM_LOG_MOD_CUSTOM(TESTMOD_LOG_FLAG_PACKET_TRACE_VERBOSE, "PACKET_TRACE_VERBOSE", __VA_ARGS__)
/** Log a module-level packet_trace_verbose with ratelimiting */
#define TESTMOD_LOG_MOD_RL_PACKET_TRACE_VERBOSE(_rl, _time, ...)           \
    AIM_LOG_MOD_RL_CUSTOM(TESTMOD_LOG_FLAG_PACKET_TRACE_VERBOSE, "PACKET_TRACE_VERBOSE", _rl, _time, __VA_ARGS__)

/** Log a module-level entry_trace */
#define TESTMOD_LOG_MOD_ENTRY_TRACE(...) \
    AIM_LOG_MOD_CUSTOM(TESTMOD_LOG_FLAG_ENTRY_TRACE, "ENTRY_TRACE", __VA_ARGS__)
/** Log a module-level entry_trace with ratelimiting */
#define TESTMOD_LOG_MOD_RL_ENTRY_TRACE(_rl, _time, ...)           \
    AIM_LOG_MOD_RL_CUSTOM(TESTMOD_LOG_FLAG_ENTRY_TRACE, "ENTRY_TRACE", _rl, _time, __VA_ARGS__)

/******************************************************************************
 *
 * Custom Object Log Macros
 *
 *****************************************************************************/

/** Log an object-level packet_trace */
#define TESTMOD_LOG_OBJ_PACKET_TRACE(_obj, ...) \
    AIM_LOG_OBJ_CUSTOM(_obj, TESTMOD_LOG_FLAG_PACKET_TRACE, "PACKET_TRACE", __VA_ARGS__)
/** Log an object-level packet_trace with ratelimiting */
#define TESTMOD_LOG_OBJ_RL_PACKET_TRACE(_obj, _rl, _time, ...) \
    AIM_LOG_OBJ_RL_CUSTOM(_obj, TESTMOD_LOG_FLAG_PACKET_TRACE, "PACKET_TRACE", _rl, _time, __VA_ARGS__)

/** Log an object-level packet_trace_verbose */
#define TESTMOD_LOG_OBJ_PACKET_TRACE_VERBOSE(_obj, ...) \
    AIM_LOG_OBJ_CUSTOM(_obj, TESTMOD_LOG_FLAG_PACKET_TRACE_VERBOSE, "PACKET_TRACE_VERBOSE", __VA_ARGS__)
/** Log an object-level packet_trace_verbose with ratelimiting */
#define TESTMOD_LOG_OBJ_RL_PACKET_TRACE_VERBOSE(_obj, _rl, _time, ...) \
    AIM_LOG_OBJ_RL_CUSTOM(_obj, TESTMOD_LOG_FLAG_PACKET_TRACE_VERBOSE, "PACKET_TRACE_VERBOSE", _rl, _time, __VA_ARGS__)

/** Log an object-level entry_trace */
#define TESTMOD_LOG_OBJ_ENTRY_TRACE(_obj, ...) \
    AIM_LOG_OBJ_CUSTOM(_obj, TESTMOD_LOG_FLAG_ENTRY_TRACE, "ENTRY_TRACE", __VA_ARGS__)
/** Log an object-level entry_trace with ratelimiting */
#define TESTMOD_LOG_OBJ_RL_ENTRY_TRACE(_obj, _rl, _time, ...) \
    AIM_LOG_OBJ_RL_CUSTOM(_obj, TESTMOD_LOG_FLAG_ENTRY_TRACE, "ENTRY_TRACE", _rl, _time, __VA_ARGS__)

/******************************************************************************
 *
 * Default Macro Mappings 
 *
 *****************************************************************************/
#ifdef AIM_LOG_OBJ_DEFAULT

/** PACKET_TRACE -> OBJ_PACKET_TRACE */
#define TESTMOD_LOG_PACKET_TRACE TESTMOD_LOG_OBJ_PACKET_TRACE
/** RL_PACKET_TRACE -> OBJ_RL_PACKET_TRACE */
#define TESTMOD_LOG_RL_PACKET_TRACE TESTMOD_LOG_RL_OBJ_PACKET_TRACE


/** PACKET_TRACE_VERBOSE -> OBJ_PACKET_TRACE_VERBOSE */
#define TESTMOD_LOG_PACKET_TRACE_VERBOSE TESTMOD_LOG_OBJ_PACKET_TRACE_VERBOSE
/** RL_PACKET_TRACE_VERBOSE -> OBJ_RL_PACKET_TRACE_VERBOSE */
#define TESTMOD_LOG_RL_PACKET_TRACE_VERBOSE TESTMOD_LOG_RL_OBJ_PACKET_TRACE_VERBOSE


/** ENTRY_TRACE -> OBJ_ENTRY_TRACE */
#define TESTMOD_LOG_ENTRY_TRACE TESTMOD_LOG_OBJ_ENTRY_TRACE
/** RL_ENTRY_TRACE -> OBJ_RL_ENTRY_TRACE */
#define TESTMOD_LOG_RL_ENTRY_TRACE TESTMOD_LOG_RL_OBJ_ENTRY_TRACE


#else

/** PACKET_TRACE -> MOD_PACKET_TRACE */
#define TESTMOD_LOG_PACKET_TRACE TESTMOD_LOG_MOD_PACKET_TRACE
/** RL_PACKET_TRACE -> MOD_RL_PACKET_TRACE */
#define TESTMOD_LOG_RL_PACKET_TRACE TESTMOD_LOG_MOD_RL_PACKET_TRACE

/** PACKET_TRACE_VERBOSE -> MOD_PACKET_TRACE_VERBOSE */
#define TESTMOD_LOG_PACKET_TRACE_VERBOSE TESTMOD_LOG_MOD_PACKET_TRACE_VERBOSE
/** RL_PACKET_TRACE_VERBOSE -> MOD_RL_PACKET_TRACE_VERBOSE */
#define TESTMOD_LOG_RL_PACKET_TRACE_VERBOSE TESTMOD_LOG_MOD_RL_PACKET_TRACE_VERBOSE

/** ENTRY_TRACE -> MOD_ENTRY_TRACE */
#define TESTMOD_LOG_ENTRY_TRACE TESTMOD_LOG_MOD_ENTRY_TRACE
/** RL_ENTRY_TRACE -> MOD_RL_ENTRY_TRACE */
#define TESTMOD_LOG_RL_ENTRY_TRACE TESTMOD_LOG_MOD_RL_ENTRY_TRACE

#endif
/* <auto.end.aim_custom_log_macro(ALL).header> */


#define LOG_ERROR AIM_LOG_ERROR
#define LOG_WARN AIM_LOG_WARN
#define LOG_INFO AIM_LOG_INFO
#define LOG_VERBOSE AIM_LOG_VERBOSE
#define LOG_TRACE AIM_LOG_TRACE
#define LOG_ENTRY_TRACE TESTMOD_LOG_ENTRY_TRACE
#define LOG_PACKET_TRACE TESTMOD_LOG_PACKET_TRACE
#define LOG_PACKET_TRACE_VERBOSE TESTMOD_LOG_PACKET_TRACE_VERBOSE

#endif /* __TESTMOD_LOG_H__ */
