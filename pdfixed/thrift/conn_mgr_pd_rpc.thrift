include "res.thrift"

namespace py conn_mgr_pd_rpc
namespace cpp conn_mgr_pd_rpc
namespace c_glib conn_mgr_pd_rpc

service conn_mgr {
    # Test echo interface
    void echo(1:string s);

    void init();

    void cleanup();

    res.SessionHandle_t client_init();

    i32 client_cleanup(1:res.SessionHandle_t sess_hdl);

    i32 begin_txn(1:res.SessionHandle_t sess_hdl, 2:bool isAtomic, 3:bool isHighPri);

    i32 verify_txn(1:res.SessionHandle_t sess_hdl);

    i32 abort_txn(1:res.SessionHandle_t sess_hdl);

    i32 commit_txn(1:res.SessionHandle_t sess_hdl, 2:bool hwSynchronous);

    i32 complete_operations(1:res.SessionHandle_t sess_hdl);
}
