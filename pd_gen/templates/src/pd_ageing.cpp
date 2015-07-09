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

#include "nn.h"
#include "pd/pd_learning.h"
#include "pd_conn_mgr.h"
#include <nanomsg/pubsub.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>

#define NUM_DEVICES 256

extern pd_conn_mgr_t *conn_mgr_state;
extern int *my_devices;

struct AgeingCb {
  p4_pd_notify_timeout_cb cb_fn;
  void *cb_cookie;
};

struct AgeingState {
  // maps table id to call back
  std::unordered_map<int, AgeingCb> cbs;
};


static AgeingState *device_state[NUM_DEVICES];

typedef struct {
  int switch_id;
  uint64_t buffer_id;
  int table_id;
  unsigned int num_entries;
} __attribute__((packed)) ageing_hdr_t;

static void ageing_listener(const char *ageing_addr) {
  nn::socket s(AF_SP, NN_SUB);
  s.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
  s.connect(ageing_addr);

  struct nn_msghdr msghdr;
  struct nn_iovec iov[2];
  ageing_hdr_t ageing_hdr;
  char data[4096];
  iov[0].iov_base = &ageing_hdr;
  iov[0].iov_len = sizeof(ageing_hdr);
  iov[1].iov_base = data;
  iov[1].iov_len = sizeof(data); // apparently only max size needed ?
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_iov = iov;
  msghdr.msg_iovlen = 2;

  while(s.recvmsg(&msghdr, 0) >= 0) {
    std::cout << "I received " << ageing_hdr.num_entries << " expired hanldes "
	      << "for table " << ageing_hdr.table_id << std::endl;
    // const AgeingState *state = device_state[ageing_hdr.switch_id];
    const AgeingState *state = device_state[0];
    const AgeingCb &cb = state->cbs.find(ageing_hdr.table_id)->second;
    uint64_t *handles = (uint64_t *) &data;
    for(unsigned int i = 0; i < ageing_hdr.num_entries; i++) {
      cb.cb_fn(handles[i], cb.cb_cookie);
    }
  }
}

extern "C" {

p4_pd_status_t ${pd_prefix}ageing_set_cb(int dev_id, int table_id,
					 p4_pd_notify_timeout_cb cb_fn,
					 void *cb_cookie) {
  AgeingState *state = device_state[dev_id];
  state->cbs[table_id] = {cb_fn, cb_cookie};
  return 0;
}

p4_pd_status_t ${pd_prefix}ageing_new_device(int dev_id) {
  device_state[dev_id] = new AgeingState();
  return 0;
}

p4_pd_status_t ${pd_prefix}ageing_remove_device(int dev_id) {
  assert(device_state[dev_id]);
  delete device_state[dev_id];
  return 0;
}

p4_pd_status_t ${pd_prefix}start_ageing_listener(const char *ageing_addr) {
  std::thread t(ageing_listener, ageing_addr);

  t.detach();

  return 0;
}

}
