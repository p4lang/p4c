#include "pd/pd.h"

#include "pd_conn_mgr.h"

pd_conn_mgr_t *conn_mgr_state;

extern "C" {

p4_pd_status_t ${pd_prefix}start_learning_listener(const char *learning_addr);
p4_pd_status_t ${pd_prefix}learning_new_device(int dev_id);

p4_pd_status_t ${pd_prefix}init(const char *learning_addr) {
  conn_mgr_state = pd_conn_mgr_create();
  if(learning_addr) {
    ${pd_prefix}start_learning_listener(learning_addr);
  }
  return 0;
}

p4_pd_status_t ${pd_prefix}assign_device(int dev_id, int rpc_port_num) {
  ${pd_prefix}learning_new_device(dev_id);
  return pd_conn_mgr_client_init(conn_mgr_state, dev_id, rpc_port_num);
}


}
