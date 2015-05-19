#include "pd/pd_static.h"
#include "pd_conn_mgr.h"

#include <cassert>
#include <iostream>

static uint64_t session_hdl = 0;

pd_conn_mgr_t *conn_mgr_state;

extern "C" {

p4_pd_status_t
p4_pd_init(void) {
  conn_mgr_state = pd_conn_mgr_create();
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

p4_pd_status_t
p4_pd_load_new_config(p4_pd_sess_hdl_t shdl, uint8_t dev_id,
		      const char *config_str) {
  RuntimeClient *client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  assert(client);
  try {
    client->bm_load_new_config(std::string(config_str));
  } catch(InvalidSwapOperation& iso) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(iso.what)->second;
    std::cout << "Invalid swap operation (" << iso.what << "): "
	      << what << std::endl;
    return iso.what;
  }
  return 0;
}

p4_pd_status_t
p4_pd_swap_configs(p4_pd_sess_hdl_t shdl, uint8_t dev_id) {
  RuntimeClient *client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  assert(client);
  try {
    client->bm_swap_configs();
  } catch(InvalidSwapOperation& iso) {
    const char *what =
      _TableOperationErrorCode_VALUES_TO_NAMES.find(iso.what)->second;
    std::cout << "Invalid swap operation (" << iso.what << "): "
	      << what << std::endl;
    return iso.what;
  }
  return 0;
}

}
