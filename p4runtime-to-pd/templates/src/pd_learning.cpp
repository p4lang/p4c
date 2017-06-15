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

#include <iostream>
#include <mutex>
#include <map>
#include <cstring>

#include "pd/pd_learning.h"
#include "pd_client.h"

#define NUM_DEVICES 256

extern int *my_devices;

namespace {

//:: for lq_name, lq in learn_quantas.items():

struct ${lq.cname}Client {
  ${pd_prefix}${lq.cname}_digest_notify_cb cb_fn;
  void *cb_cookie;
};

//:: #endfor

struct LearnState {
//:: for lq_name, lq in learn_quantas.items():
  std::map<p4_pd_sess_hdl_t, ${lq.cname}Client> ${lq.cname}_clients;
  std::mutex ${lq.cname}_mutex;
//:: #endfor
};

LearnState *device_state[NUM_DEVICES];

template <int L>
void bytes_to_field(const char *bytes, char *field) {
  std::memcpy(field, bytes, L);
}

template <>
void bytes_to_field<1>(const char *bytes, char *field) {
  field[0] = bytes[0];
}

template <>
void bytes_to_field<2>(const char *bytes, char *field) {
#ifdef HOST_BYTE_ORDER_CALLER
  field[0] = bytes[1];
  field[1] = bytes[0];
#else
  field[0] = bytes[0];
  field[1] = bytes[1];
#endif
}

template <>
void bytes_to_field<3>(const char *bytes, char *field) {
#ifdef HOST_BYTE_ORDER_CALLER
  field[0] = bytes[2];
  field[1] = bytes[1];
  field[2] = bytes[0];
  field[3] = '\0';
#else
  field[0] = '\0';
  std::memcpy(field + 1, bytes, 3);
#endif
}

template <>
void bytes_to_field<4>(const char *bytes, char *field) {
#ifdef HOST_BYTE_ORDER_CALLER
  field[0] = bytes[3];
  field[1] = bytes[2];
  field[2] = bytes[1];
  field[3] = bytes[0];
#else
  std::memcpy(field, bytes, 4);
#endif
}

typedef struct {
  char sub_topic[4];
  int switch_id;
  int cxt_id;
  int list_id;
  unsigned long long buffer_id;
  unsigned int num_samples;
  char _padding[4];
} __attribute__((packed)) learn_hdr_t;


//:: for lq_name, lq in learn_quantas.items():
void ${lq.cname}_handle_learn_msg(const learn_hdr_t *hdr, const char *data) {
  LearnState *state = device_state[hdr->switch_id];
  auto lock = std::unique_lock<std::mutex>(state->${lq.cname}_mutex);
  ${pd_prefix}${lq.cname}_digest_msg_t msg;
  msg.dev_tgt.device_id = static_cast<uint8_t>(hdr->switch_id);
  msg.num_entries = hdr->num_samples;
  std::unique_ptr<${pd_prefix}${lq.cname}_digest_entry_t []> buf(
      new ${pd_prefix}${lq.cname}_digest_entry_t[hdr->num_samples]);
  const char *data_ = data;
  char *buf_;
  for(size_t i = 0; i < hdr->num_samples; i++){
//::   for name, bit_width in lq.fields:
//::     c_name = get_c_name(name)
//::     width = (bit_width + 7) / 8
//::     if width > 4:
    buf_ = reinterpret_cast<char *>(buf[i].${c_name});
//::     else:
    buf_ = reinterpret_cast<char *>(&buf[i].${c_name});
//::     #endif
    bytes_to_field<${width}>(data_, buf_);
    data_ += ${width};
//::   #endfor	
  }
  msg.entries = buf.get();
  msg.buffer_id = hdr->buffer_id;
  for(const auto &it : state->${lq.cname}_clients) {
    it.second.cb_fn(it.first, &msg, it.second.cb_cookie);
  }
}

//:: #endfor

}  // namespace

extern "C" {

p4_pd_status_t ${pd_prefix}set_learning_timeout
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t device_id,
 uint64_t usecs
) {
  (void) sess_hdl;
  assert(my_devices[device_id]);
  try {
    // in bmv2, there can be a different timeout for each learn list, so this
    // RPC function is called for each learn list
    // bmv2 also takes a timeout in ms, thus the "/ 1000"
//:: for lq_name, lq in learn_quantas.items():
    pd_client(device_id).c->bm_learning_set_timeout(0, ${lq.id_}, usecs / 1000);
//:: #endfor
  } catch (InvalidLearnOperation &ilo) {
    const char *what =
      _LearnOperationErrorCode_VALUES_TO_NAMES.find(ilo.code)->second;
    std::cout << "Invalid learn operation (" << ilo.code << "): "
              << what << std::endl;
    return ilo.code;
  }
  return 0;
}

//:: for lq_name, lq in learn_quantas.items():

p4_pd_status_t ${pd_prefix}${lq.cname}_register
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t device_id,
 ${pd_prefix}${lq.cname}_digest_notify_cb cb_fn,
 void *cb_fn_cookie
) {
  LearnState *state = device_state[device_id];
  auto &clients = state->${lq.cname}_clients;
  std::unique_lock<std::mutex> lock(state->${lq.cname}_mutex);
  ${lq.cname}Client new_client = {cb_fn, cb_fn_cookie};
  const auto &ret = clients.insert(std::make_pair(sess_hdl, new_client));
  assert(ret.second); // no duplicates
  return 0;
}

p4_pd_status_t ${pd_prefix}${lq.cname}_deregister
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t device_id
) {
  LearnState *state = device_state[device_id];
  auto &clients = state->${lq.cname}_clients;
  std::unique_lock<std::mutex> lock(state->${lq.cname}_mutex);
  assert(clients.erase(sess_hdl) == 1);
  return 0;
}

p4_pd_status_t ${pd_prefix}${lq.cname}_notify_ack
(
 p4_pd_sess_hdl_t sess_hdl,
 ${pd_prefix}${lq.cname}_digest_msg_t *msg
) {
  assert(my_devices[msg->dev_tgt.device_id]);
  pd_client(msg->dev_tgt.device_id).c->bm_learning_ack_buffer(
      0, ${lq.id_}, msg->buffer_id);
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

void ${pd_prefix}learning_notification_cb(const char *hdr, const char *data) {
  const learn_hdr_t *learn_hdr = reinterpret_cast<const learn_hdr_t *>(hdr);
  // std::cout << "I received " << learn_hdr->num_samples << " samples"
  //           << std::endl;
  switch(learn_hdr->list_id) {
//:: for lq_name, lq in learn_quantas.items():
  case ${lq.id_}:
    ${lq.cname}_handle_learn_msg(learn_hdr, data);
    break;
//:: #endfor
  default:
    assert(0 && "invalid lq id");
    break;
  }  
}

}
