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

#include "bm_sim/ageing.h"

template <typename Transport>
AgeingWriterImpl<Transport>::AgeingWriterImpl(const std::string &transport_name)
  : transport_instance(std::unique_ptr<Transport>(new Transport()))
{
  // should not be doing this in the constructor ?
  transport_instance->open(transport_name);
}

template <typename Transport>
int AgeingWriterImpl<Transport>::send(const char *buffer, size_t len) const 
{
  return transport_instance->send(buffer, len);
}

template <typename Transport>
int AgeingWriterImpl<Transport>::send_msgs(
    const std::initializer_list<TransportIface::MsgBuf> &msgs
) const
{
  return transport_instance->send_msgs(msgs);
}

// explicit template instantiation
template class AgeingWriterImpl<TransportNanomsg>;

AgeingMonitor::AgeingMonitor(std::shared_ptr<AgeingWriter> writer,
			     unsigned int sweep_interval_ms)
  : writer(writer), sweep_interval_ms(sweep_interval_ms)
{
  sweep_thread = std::thread(&AgeingMonitor::sweep_loop, this);
}

AgeingMonitor::~AgeingMonitor()
{
  {
    std::unique_lock<std::mutex> lock(mutex);
    stop_sweep_thread = true;
  }
  stop_condvar.notify_one();
  sweep_thread.join();
}

void
AgeingMonitor::add_table(MatchTableAbstract *table)
{
  tables_with_ageing.insert(std::make_pair(table->get_id(), TableData(table)));
}

void
AgeingMonitor::set_sweep_interval(unsigned int ms)
{
  sweep_interval_ms = ms;
}

void
AgeingMonitor::reset_state()
{
  std::unique_lock<std::mutex> lock(mutex);
  entries.clear();
  entries_tmp.clear();
  buffer_id = 0;
  for(auto &entry : tables_with_ageing) {
    TableData &data = entry.second;
    data.prev_sweep_entries.clear();
  }
}

void
AgeingMonitor::sweep_loop()
{
  using std::chrono::milliseconds;
  auto tp = clock::now();
  std::unique_lock<std::mutex> lock(mutex);
  while(!stop_sweep_thread) {
    do_sweep();
    milliseconds interval(sweep_interval_ms);
    tp += interval;
    stop_condvar.wait_until(lock, tp);
  }
}

void
AgeingMonitor::do_sweep()
{
  for(auto &entry : tables_with_ageing) {
    TableData &data = entry.second;
    const MatchTableAbstract *t = data.table;
    auto &prev_sweep_entries = data.prev_sweep_entries;
    t->sweep_entries(entries_tmp);
    if(entries_tmp.empty()) {
      prev_sweep_entries.clear();
      continue;
    }

    for(entry_handle_t handle : entries_tmp) {
      if(prev_sweep_entries.find(handle) == prev_sweep_entries.end()) {
	entries.push_back(handle);
      }
    }
    entries_tmp.clear();
    prev_sweep_entries.clear();

    if(entries.empty()) continue;

    for(entry_handle_t handle : entries) {
      prev_sweep_entries.insert(handle);
    }

    unsigned int num_entries = static_cast<unsigned int>(entries.size());
    unsigned int size =
      static_cast<unsigned int>(num_entries * sizeof(entry_handle_t));
    msg_hdr_t msg_hdr = {0, buffer_id++, entry.first, num_entries};
    TransportIface::MsgBuf buf_hdr = {(char *) &msg_hdr, sizeof(msg_hdr)};
    TransportIface::MsgBuf buf_entries = {(char *) entries.data(), size};
    writer->send_msgs({buf_hdr, buf_entries});
    entries.clear();
  }
}
