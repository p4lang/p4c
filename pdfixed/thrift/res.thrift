namespace py res_pd_rpc
namespace cpp res_pd_rpc
namespace c_glib res_pd_rpc

typedef i32 SessionHandle_t
struct DevTarget_t {
  1: required byte dev_id;
  2: required i16 dev_pipe_id;
}
