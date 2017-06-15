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

#include <bm/pdfixed/int/pd_notifications.h>

#include <cstring>

#include "pd/pd.h"
#include "pd_client.h"

#define NUM_DEVICES 256

int *my_devices;

extern "C" {

void ${pd_prefix}learning_notification_cb(const char *hdr, const char *data);
p4_pd_status_t ${pd_prefix}learning_new_device(int dev_id);
p4_pd_status_t ${pd_prefix}learning_remove_device(int dev_id);

void ${pd_prefix}ageing_notification_cb(const char *hdr, const char *data);
p4_pd_status_t ${pd_prefix}ageing_new_device(int dev_id);
p4_pd_status_t ${pd_prefix}ageing_remove_device(int dev_id);

p4_pd_status_t ${pd_prefix}init(void) {
  my_devices = (int *) calloc(NUM_DEVICES, sizeof(int));
  return 0;
}

p4_pd_status_t ${pd_prefix}assign_device(int dev_id,
                                         const char *notifications_addr,
                                         int rpc_port_num) {
  assert(!my_devices[dev_id]);
  ${pd_prefix}learning_new_device(dev_id);
  ${pd_prefix}ageing_new_device(dev_id);
  pd_notifications_add_device(dev_id, notifications_addr,
                              ${pd_prefix}ageing_notification_cb,
                              ${pd_prefix}learning_notification_cb);
  my_devices[dev_id] = 1;
  return conn_mgr_state->client_init(dev_id, rpc_port_num);
}

p4_pd_status_t ${pd_prefix}remove_device(int dev_id) {
  assert(my_devices[dev_id]);
  pd_notifications_remove_device(dev_id);
  ${pd_prefix}learning_remove_device(dev_id);
  ${pd_prefix}ageing_remove_device(dev_id);
  my_devices[dev_id] = 0;
  return conn_mgr_state->client_close(dev_id);
}

}
