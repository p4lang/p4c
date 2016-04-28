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

#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <unordered_set>
#include <string>
#include <vector>

#include "match_tables.h"
#include "packet.h"
#include "transport.h"

namespace bm {

class AgeingMonitor {
 public:
  typedef uint64_t buffer_id_t;
  typedef Packet::clock clock;

  typedef struct {
    char sub_topic[4];
    int switch_id;
    int cxt_id;
    uint64_t buffer_id;
    int table_id;
    unsigned int num_entries;
    char _padding[4];  // the header size for notifications is always 32 bytes
  } __attribute__((packed)) msg_hdr_t;

  AgeingMonitor(int device_id, int cxt_id,
                std::shared_ptr<TransportIface> writer,
                unsigned int sweep_interval_ms = 1000u);

  ~AgeingMonitor();

  void add_table(MatchTableAbstract *table);

  void set_sweep_interval(unsigned int ms);

  void reset_state();

 private:
  void sweep_loop();
  void do_sweep();

 private:
  struct TableData {
    explicit TableData(MatchTableAbstract *table)
      : table(table) { }

    TableData(const TableData &other) = delete;
    TableData &operator=(const TableData &other) = delete;

    TableData(TableData &&other) /*noexcept*/ = default;
    TableData &operator=(TableData &&other) /*noexcept*/ = default;

    MatchTableAbstract *table{nullptr};
    std::unordered_set<entry_handle_t> prev_sweep_entries{};
  };

 private:
  mutable std::mutex mutex{};

  std::map<p4object_id_t, TableData> tables_with_ageing{};

  int device_id{};
  int cxt_id{};

  std::shared_ptr<TransportIface> writer{nullptr};

  std::atomic<unsigned int> sweep_interval_ms{0};

  std::vector<entry_handle_t> entries{};
  std::vector<entry_handle_t> entries_tmp{};
  buffer_id_t buffer_id{0};

  std::thread sweep_thread{};
  bool stop_sweep_thread{false};
  mutable std::condition_variable stop_condvar{};
};

}  // namespace bm

#endif  // BM_BM_SIM_AGEING_H_
