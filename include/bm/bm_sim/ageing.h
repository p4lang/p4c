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

#ifndef BM_BM_SIM_AGEING_H_
#define BM_BM_SIM_AGEING_H_

#include <memory>

#include "transport.h"

namespace bm {

class MatchTableAbstract;

class AgeingMonitorIface {
 public:
  typedef struct {
    char sub_topic[4];
    int switch_id;
    int cxt_id;
    uint64_t buffer_id;
    int table_id;
    unsigned int num_entries;
    char _padding[4];  // the header size for notifications is always 32 bytes
  } __attribute__((packed)) msg_hdr_t;

  virtual ~AgeingMonitorIface() { }

  virtual void add_table(MatchTableAbstract *table) = 0;

  virtual void set_sweep_interval(unsigned int ms) = 0;

  virtual void reset_state() = 0;

  static std::unique_ptr<AgeingMonitorIface> make(
      int device_id, int cxt_id, std::shared_ptr<TransportIface> writer,
      unsigned int sweep_interval_ms = 1000u);
};

}  // namespace bm

#endif  // BM_BM_SIM_AGEING_H_
