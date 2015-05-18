#include "pd/pd_static.h"
#include "pd_conn_mgr.h"

#include <assert.h>

static uint64_t session_hdl = 0;

extern "C" {

p4_pd_status_t
p4_pd_init(void) {
  return 0;
}

void
p4_pd_cleanup(void) {
  
}

p4_pd_status_t
p4_pd_client_init(p4_pd_sess_hdl_t *sess_hdl, uint32_t max_txn_size) {
  *sess_hdl = session_hdl++;
  return 0;
}

p4_pd_status_t
p4_pd_client_cleanup(p4_pd_sess_hdl_t sess_hdl) {
  return 0;
}

p4_pd_status_t
p4_pd_begin_txn(p4_pd_sess_hdl_t shdl, bool isAtomic, bool isHighPri) {
  return 0;
}

p4_pd_status_t
p4_pd_verify_txn(p4_pd_sess_hdl_t shdl) {
  return 0;
}

p4_pd_status_t
p4_pd_abort_txn(p4_pd_sess_hdl_t shdl) {
  return 0;
}

p4_pd_status_t
p4_pd_commit_txn(p4_pd_sess_hdl_t shdl, bool hwSynchronous, bool *sendRsp) {
  return 0;
}

p4_pd_status_t
p4_pd_complete_operations(p4_pd_sess_hdl_t shdl) {
  return 0;
}

}
