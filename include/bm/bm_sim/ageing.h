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
#include <string>

#include "device_id.h"
#include "named_p4object.h"
#include "transport.h"

namespace bm {

class MatchTableAbstract;

class AgeingMonitorIface {
 public:
  // the header size for notifications is always 32 bytes
  struct msg_hdr_t {
    char sub_topic[4];
    s_device_id_t switch_id;
    s_cxt_id_t cxt_id;
    uint64_t buffer_id;
    int table_id;
    unsigned int num_entries;
  } __attribute__((packed));

  virtual ~AgeingMonitorIface() { }

  virtual void add_table(MatchTableAbstract *table) = 0;

  virtual void set_sweep_interval(unsigned int ms) = 0;

  virtual unsigned int get_sweep_interval() = 0;

  virtual void reset_state() = 0;

  //! Returns an empty string if table doesn't support idle timeout; this is
  //! useful to decode notifications.
  //! Not thread-safe but can be called safely after the map has been populated
  //! by P4Objects::init_objects, including as idle notifications are being
  //! generated.
  virtual std::string get_table_name_from_id(p4object_id_t id) const = 0;

  static std::unique_ptr<AgeingMonitorIface> make(
      device_id_t device_id, cxt_id_t cxt_id,
      std::shared_ptr<TransportIface> writer,
      unsigned int sweep_interval_ms = 1000u);
};

}  // namespace bm

#endif  // BM_BM_SIM_AGEING_H_
