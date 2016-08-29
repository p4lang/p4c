#include "conn_mgr.h"
#include <bm/pdfixed/pd_static.h>

#include <iostream>

using namespace  ::conn_mgr_pd_rpc;
using namespace  ::res_pd_rpc;

class conn_mgrHandler : virtual public conn_mgrIf {
public:
  conn_mgrHandler() {

  }

  void echo(const std::string& s) {
    std::cerr << "Echo: " << s << std::endl;
  }

  void init(){
    std::cerr << "In init\n";
    p4_pd_init();
  }

  void cleanup(){
    std::cerr << "In cleanup\n";
    p4_pd_cleanup();
  }

  SessionHandle_t client_init(){
    std::cerr << "In client_init\n";

    p4_pd_sess_hdl_t sess_hdl;
    p4_pd_client_init(&sess_hdl);
    return sess_hdl;
  }

  int32_t client_cleanup(const SessionHandle_t sess_hdl){
    std::cerr << "In client_cleanup\n";

    return p4_pd_client_cleanup(sess_hdl);
  }

  int32_t begin_txn(const SessionHandle_t sess_hdl, const bool isAtomic, const bool isHighPri){
    std::cerr << "In begin_txn\n";

    return p4_pd_begin_txn(sess_hdl, isAtomic, isHighPri);
  }

  int32_t verify_txn(const SessionHandle_t sess_hdl){
    std::cerr << "In verify_txn\n";

    return p4_pd_verify_txn(sess_hdl);
  }

  int32_t abort_txn(const SessionHandle_t sess_hdl){
    std::cerr << "In abort_txn\n";

    return p4_pd_abort_txn(sess_hdl);
  }

  int32_t commit_txn(const SessionHandle_t sess_hdl, const bool hwSynchronous){
    std::cerr << "In commit_txn\n";

    return p4_pd_commit_txn(sess_hdl, hwSynchronous);
  }

  int32_t complete_operations(const SessionHandle_t sess_hdl){
    std::cerr << "In complete_operations\n";

    return p4_pd_complete_operations(sess_hdl);
  }
};
