namespace py res_pd_rpc
namespace cpp res_pd_rpc
namespace c_glib res_pd_rpc

typedef i32 SessionHandle_t
struct DevTarget_t {
  1: required byte dev_id;
  2: required i16 dev_pipe_id;
}

enum P4LogLevel_t {
 P4_LOG_LEVEL_NONE = 0, # otherwise will it start from 1?
 P4_LOG_LEVEL_FATAL,
 P4_LOG_LEVEL_ERROR,
 P4_LOG_LEVEL_WARN,
 P4_LOG_LEVEL_INFO,
 P4_LOG_LEVEL_VERBOSE,
 P4_LOG_LEVEL_TRACE
}
