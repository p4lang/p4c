/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <cstring>

#include "pd/pd.h"

#include "pd_conn_mgr.h"

extern pd_conn_mgr_t *conn_mgr_state;

#define NUM_DEVICES 256

int *my_devices;

extern "C" {

p4_pd_status_t ${pd_prefix}start_learning_listener(const char *learning_addr);
p4_pd_status_t ${pd_prefix}learning_new_device(int dev_id);
p4_pd_status_t ${pd_prefix}learning_remove_device(int dev_id);

p4_pd_status_t ${pd_prefix}ageing_new_device(int dev_id);
p4_pd_status_t ${pd_prefix}ageing_remove_device(int dev_id);
p4_pd_status_t ${pd_prefix}start_ageing_listener(const char *ageing_addr);

p4_pd_status_t ${pd_prefix}init(const char *learning_addr, const char *ageing_addr) {
  my_devices = (int *) calloc(NUM_DEVICES, sizeof(int));
  if(learning_addr) {
    ${pd_prefix}start_learning_listener(learning_addr);
  }
  if(ageing_addr) {
    ${pd_prefix}start_ageing_listener(ageing_addr);
  }
  return 0;
}

p4_pd_status_t ${pd_prefix}assign_device(int dev_id, int rpc_port_num) {
  assert(!my_devices[dev_id]);
  ${pd_prefix}learning_new_device(dev_id);
  ${pd_prefix}ageing_new_device(dev_id);
  my_devices[dev_id] = 1;
  return pd_conn_mgr_client_init(conn_mgr_state, dev_id, rpc_port_num);
}

p4_pd_status_t ${pd_prefix}remove_device(int dev_id) {
  assert(my_devices[dev_id]);
  ${pd_prefix}learning_remove_device(dev_id);
  ${pd_prefix}ageing_remove_device(dev_id);
  my_devices[dev_id] = 0;
  return pd_conn_mgr_client_close(conn_mgr_state, dev_id);
}

}
