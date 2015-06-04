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
#include<map>

#define NUM_DEVICES 256

extern pd_conn_mgr_t *conn_mgr_state;
extern int *my_devices;

//:: for lq_name, lq in learn_quantas.items():
//::   lq_name = get_c_name(lq_name)

struct ${lq_name}Client {
  ${pd_prefix}${lq_name}_digest_notify_cb cb_fn;
  void *cb_cookie;
};

//:: #endfor

struct LearnState {
//:: for lq_name, lq in learn_quantas.items():
//::   lq_name = get_c_name(lq_name)
  std::map<p4_pd_sess_hdl_t, ${lq_name}Client> ${lq_name}_clients;
  std::mutex ${lq_name}_mutex;
//:: #endfor
};

static LearnState *device_state[NUM_DEVICES];

typedef struct {
  int switch_id;
  int list_id;
  unsigned long long buffer_id;
  unsigned int num_samples;
} __attribute__((packed)) learn_hdr_t;

static void learning_listener(nn::socket s) {
  struct nn_msghdr msghdr;
  struct nn_iovec iov[2];
  learn_hdr_t learn_hdr;
  char data[4096];
  iov[0].iov_base = &learn_hdr;
  iov[0].iov_len = sizeof(learn_hdr);
  iov[1].iov_base = data;
  iov[1].iov_len = sizeof(data); // apparently only max size needed ?
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_iov = iov;
  msghdr.msg_iovlen = 2;

  LearnState *state;
  std::unique_lock<std::mutex> lock;

  while(s.recvmsg(&msghdr, 0) >= 0) {
    std::cout << "I received " << learn_hdr.num_samples << " samples"
	      << std::endl;

    // right now I am just sending the data as is, this is wrong for those damn
    // 3 byte fields which are padded to 4 bytes in the PD
    switch(learn_hdr.list_id) {
//:: for lq_name, lq in learn_quantas.items():
//::   lq_name = get_c_name(lq_name)
    case ${lq.id_}:
      state = device_state[learn_hdr.switch_id];
      lock = std::unique_lock<std::mutex>(state->${lq_name}_mutex);
      ${pd_prefix}${lq_name}_digest_msg_t ${lq_name}_msg;
      ${lq_name}_msg.dev_tgt.device_id = (uint8_t) learn_hdr.switch_id;
      ${lq_name}_msg.num_entries = learn_hdr.num_samples;
      ${lq_name}_msg.entries = (${pd_prefix}${lq_name}_digest_entry_t *) data;
      ${lq_name}_msg.buffer_id = learn_hdr.buffer_id;
      for(const auto &it : state->${lq_name}_clients) {
	it.second.cb_fn(it.first, &${lq_name}_msg, it.second.cb_cookie);
      }
      break;
//:: #endfor
    default:
      assert(0 && "invalid lq id");
      break;
    }
  }
}

extern "C" {

//:: for lq_name, lq in learn_quantas.items():
//::   lq_name = get_c_name(lq_name)

p4_pd_status_t ${pd_prefix}${lq_name}_register
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t device_id,
 ${pd_prefix}${lq_name}_digest_notify_cb cb_fn,
 void *cb_fn_cookie
) {
  LearnState *state = device_state[device_id];
  auto &clients = state->${lq_name}_clients;
  std::unique_lock<std::mutex> lock(state->${lq_name}_mutex);
  ${lq_name}Client new_client = {cb_fn, cb_fn_cookie};
  const auto &ret = clients.insert(std::make_pair(sess_hdl, new_client));
  assert(ret.second); // no duplicates
  return 0;
}

p4_pd_status_t ${pd_prefix}${lq_name}_deregister
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t device_id
) {
  LearnState *state = device_state[device_id];
  auto &clients = state->${lq_name}_clients;
  std::unique_lock<std::mutex> lock(state->${lq_name}_mutex);
  assert(clients.erase(sess_hdl) == 0);
  return 0;
}

p4_pd_status_t ${pd_prefix}${lq_name}_notify_ack
(
 p4_pd_sess_hdl_t sess_hdl,
 ${pd_prefix}${lq_name}_digest_msg_t *msg
) {
  assert(my_devices[msg->dev_tgt.device_id]);
  pd_conn_mgr_client(conn_mgr_state, msg->dev_tgt.device_id)->bm_learning_ack_buffer(
      ${lq.id_},
      msg->buffer_id
  );
  return 0;
}

//:: #endfor

p4_pd_status_t ${pd_prefix}learning_new_device(int dev_id) {
  device_state[dev_id] = new LearnState();
  return 0;
}

p4_pd_status_t ${pd_prefix}learning_remove_device(int dev_id) {
  assert(device_state[dev_id]);
  delete device_state[dev_id];
  return 0;
}

p4_pd_status_t ${pd_prefix}start_learning_listener(const char *learning_addr) {
  nn::socket s(AF_SP, NN_SUB);
  s.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
  s.connect(learning_addr);

  std::thread t(learning_listener, std::move(s));

  t.detach();

  return 0;
}

}
