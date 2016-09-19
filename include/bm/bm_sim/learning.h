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

//! @file learning.h

#ifndef BM_BM_SIM_LEARNING_H_
#define BM_BM_SIM_LEARNING_H_

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "transport.h"
#include "phv_forward.h"

namespace bm {

class Packet;

// TODO(antonin): automate learning (i.e. make it target-independent)?

//! Enables learning in the switch. For now it is the responsibility of the
//! switch (look at the simple switch target for an example) to invoke the
//! learn() method, which will send out the learning notifications.
class LearnEngineIface {
 public:
  typedef int list_id_t;
  typedef uint64_t buffer_id_t;

  enum LearnErrorCode {
    SUCCESS = 0,
    INVALID_LIST_ID,
    ERROR
  };

  typedef struct {
    char sub_topic[4];
    int switch_id;
    int cxt_id;
    int list_id;
    uint64_t buffer_id;
    unsigned int num_samples;
    char _padding[4];  // the header size for notifications is always 32 bytes
  } __attribute__((packed)) msg_hdr_t;

  typedef std::function<void(const msg_hdr_t &, size_t,
                             std::unique_ptr<char[]>, void *)> LearnCb;

  virtual ~LearnEngineIface() { }

  virtual void list_create(list_id_t list_id, size_t max_samples = 1,
                           unsigned int timeout_ms = 1000) = 0;

  virtual void list_set_learn_writer(
      list_id_t list_id, std::shared_ptr<TransportIface> learn_writer) = 0;

  virtual void list_set_learn_cb(list_id_t list_id, const LearnCb &learn_cb,
                                 void * cookie) = 0;

  virtual void list_push_back_field(list_id_t list_id, header_id_t header_id,
                                    int field_offset) = 0;

  virtual void list_push_back_constant(list_id_t list_id,
                                       const std::string &hexstring) = 0;

  virtual void list_init(list_id_t list_id) = 0;

  virtual LearnErrorCode list_set_timeout(list_id_t list_id,
                                          unsigned int timeout_ms) = 0;

  virtual LearnErrorCode list_set_max_samples(list_id_t list_id,
                                              size_t max_samples) = 0;

  //! Performs learning on the packet. Needs to be called by the target after a
  //! learning-enabled pipeline has been applied on the packet. See the simple
  //! switch implementation for an example.
  //!
  //! The \p list_id should be mapped to a field list in the JSON fed into the
  //! switch. Since learning is still not well-standardized in P4, this process
  //! is a little bit hacky for now.
  virtual void learn(list_id_t list_id, const Packet &pkt) = 0;

  virtual LearnErrorCode ack(list_id_t list_id, buffer_id_t buffer_id,
                             int sample_id) = 0;
  virtual LearnErrorCode ack(list_id_t list_id, buffer_id_t buffer_id,
                             const std::vector<int> &sample_ids) = 0;
  virtual LearnErrorCode ack_buffer(list_id_t list_id,
                                    buffer_id_t buffer_id) = 0;

  virtual void reset_state() = 0;

  static std::unique_ptr<LearnEngineIface> make(int device_id = 0,
                                                int cxt_id = 0);
};

}  // namespace bm

#endif  // BM_BM_SIM_LEARNING_H_
