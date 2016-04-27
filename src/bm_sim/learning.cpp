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

#include <bm/bm_sim/learning.h>
#include <bm/bm_sim/logger.h>

#include <string>
#include <vector>
#include <algorithm>

#include <cassert>

namespace bm {

static_assert(sizeof(LearnEngine::msg_hdr_t) == 32u,
              "Invalid size for learning notification header");

void
LearnEngine::LearnSampleBuilder::push_back_constant(
    const ByteContainer &constant) {
  unsigned int offset = constants.size();
  constants.push_back(constant);
  LearnSampleEntry entry;
  entry.tag = LearnSampleEntry::CONSTANT;
  entry.constant.offset = offset;
  entries.push_back(entry);
}

void
LearnEngine::LearnSampleBuilder::push_back_field(header_id_t header_id,
                                                 int field_offset) {
  LearnSampleEntry entry;
  entry.tag = LearnSampleEntry::FIELD;
  entry.field.header = header_id;
  entry.field.offset = field_offset;
  entries.push_back(entry);
}

void
LearnEngine::LearnSampleBuilder::operator()(const PHV &phv,
                                            ByteContainer *sample) const {
  for (const LearnSampleEntry &entry : entries) {
    const ByteContainer *bytes = nullptr;
    switch (entry.tag) {
    case LearnSampleEntry::FIELD:
      bytes = &phv.get_field(entry.field.header,
                             entry.field.offset).get_bytes();
      break;
    case LearnSampleEntry::CONSTANT:
      bytes = &constants[entry.constant.offset];
      break;
    }
    sample->append(*bytes);
    // buffer.insert(buffer.end(), bytes->begin(), bytes->end());
  }
}

LearnEngine::LearnList::LearnList(list_id_t list_id, int device_id, int cxt_id,
                                  size_t max_samples, unsigned int timeout)
    : list_id(list_id), device_id(device_id), cxt_id(cxt_id),
      max_samples(max_samples), timeout(timeout), with_timeout(timeout > 0) { }

void
LearnEngine::LearnList::init() {
  transmit_thread = std::thread(&LearnList::buffer_transmit_loop, this);
  // transmit_thread.detach();
}

LearnEngine::LearnList::~LearnList() {
  {
    LockType lock(mutex);
    stop_transmit_thread = true;
  }
  b_can_send.notify_all();
  transmit_thread.join();
}

void
LearnEngine::LearnList::set_learn_writer(
    std::shared_ptr<TransportIface> learn_writer) {
  LockType lock(mutex);
  while (writer_busy) {
    can_change_writer.wait(lock);
  }
  learn_mode = LearnMode::WRITER;
  writer = learn_writer;
}

void
LearnEngine::LearnList::set_learn_cb(const LearnCb &learn_cb, void *cookie) {
  LockType lock(mutex);
  while (writer_busy) {
    can_change_writer.wait(lock);
  }
  learn_mode = LearnMode::CB;
  cb_fn = learn_cb;
  cb_cookie = cookie;
}

void
LearnEngine::LearnList::push_back_field(header_id_t header_id,
                                        int field_offset) {
  builder.push_back_field(header_id, field_offset);
}

void
LearnEngine::LearnList::push_back_constant(const std::string &hexstring) {
  builder.push_back_constant(ByteContainer(hexstring));
}

void
LearnEngine::LearnList::set_timeout(unsigned int timeout_ms) {
  LockType lock(mutex);
  timeout = milliseconds(timeout_ms);
  with_timeout = (timeout_ms > 0);
}

void
LearnEngine::LearnList::set_max_samples(size_t nb_samples) {
  LockType lock(mutex);
  max_samples = nb_samples;
  if (num_samples > 0 && num_samples >= max_samples) {
    process_full_buffer(lock);
  }
}

void
LearnEngine::LearnList::swap_buffers() {
  buffer_tmp.swap(buffer);
  num_samples_tmp = num_samples;
  num_samples = 0;
  buffer_id++;
}

void
LearnEngine::LearnList::process_full_buffer(LockType &lock) {
  while (buffer_tmp.size() != 0) {
    b_can_swap.wait(lock);
  }
  swap_buffers();

  b_can_send.notify_one();
}

void
LearnEngine::LearnList::add_sample(const PHV &phv) {
  static thread_local ByteContainer sample;
  sample.clear();
  builder(phv, &sample);

  LockType lock(mutex);

  const auto it = filter.find(sample);
  if (it != filter.end()) return;

  buffer.insert(buffer.end(), sample.begin(), sample.end());
  num_samples++;
  auto filter_it = filter.insert(filter.end(), std::move(sample));
  FilterPtrs &filter_ptrs = old_buffers[buffer_id];
  filter_ptrs.unacked_count++;
  filter_ptrs.buffer.push_back(filter_it);

  if (num_samples == 1 && max_samples > 1) {
    buffer_started = clock::now();
    // wake transmit thread to update cond var wait time
    b_can_send.notify_one();
  } else if (num_samples >= max_samples) {
    process_full_buffer(lock);
  }
}

void
LearnEngine::LearnList::buffer_transmit() {
  size_t num_samples_to_send;
  LockType lock(mutex);
  clock::time_point now = clock::now();
  while (!stop_transmit_thread &&
         buffer_tmp.size() == 0 &&
         (!with_timeout ||
          num_samples == 0 ||
          now < (buffer_started + timeout))) {
    if (with_timeout && num_samples > 0) {
      b_can_send.wait_until(lock, buffer_started + timeout);
    } else {
      b_can_send.wait(lock);
    }
    now = clock::now();
  }

  if (stop_transmit_thread) return;

  if (buffer_tmp.size() == 0) {  // timeout -> we need to swap here
    num_samples_to_send = num_samples;
    swap_buffers();
  } else {  // buffer full -> swapping was already done
    // maybe max_samples was modified in between, so it would not be safe to do
    // this
    // num_samples_to_send = max_samples;
    num_samples_to_send = num_samples_tmp;
  }

  last_sent = now;
  writer_busy = true;

  lock.unlock();

  msg_hdr_t msg_hdr;
  char *msg_hdr_ = reinterpret_cast<char *>(&msg_hdr);
  memset(msg_hdr_, 0, sizeof(msg_hdr));
  memcpy(msg_hdr_, "LEA|", 4);
  msg_hdr.switch_id = device_id;
  msg_hdr.cxt_id = cxt_id;
  msg_hdr.list_id = list_id;
  msg_hdr.buffer_id = buffer_id - 1;
  msg_hdr.num_samples = static_cast<unsigned int>(num_samples_to_send);

  if (learn_mode == LearnMode::WRITER) {
    // do not forget the -1 !!!
    // swap_buffers() is in charge of incrementing the count
    TransportIface::MsgBuf buf_hdr =
      {reinterpret_cast<char *>(&msg_hdr), sizeof(msg_hdr)};
    TransportIface::MsgBuf buf_samples =
      {buffer_tmp.data(), static_cast<unsigned int>(buffer_tmp.size())};

    writer->send_msgs({buf_hdr, buf_samples});  // no lock for I/O
  } else if (learn_mode == LearnMode::CB) {
    std::unique_ptr<char[]> buf(new char[buffer_tmp.size()]);
    std::copy(buffer_tmp.begin(), buffer_tmp.end(), &buf[0]);
    cb_fn(msg_hdr, buffer_tmp.size(), std::move(buf), cb_cookie);
  } else {
    assert(learn_mode == LearnMode::NONE);
  }

  lock.lock();

  writer_busy = false;
  can_change_writer.notify_all();

  buffer_tmp.clear();

  b_can_swap.notify_one();
}

void
LearnEngine::LearnList::buffer_transmit_loop() {
  while (true) {
    buffer_transmit();
    if (stop_transmit_thread) return;
  }
}

void
LearnEngine::LearnList::ack(buffer_id_t buffer_id,
                            const std::vector<int> &sample_ids) {
  LockType lock(mutex);
  auto it = old_buffers.find(buffer_id);
  // assert(it != old_buffers.end());
  // we assume that this was acked already, and simply return
  if (it == old_buffers.end())
    return;
  FilterPtrs &filter_ptrs = it->second;
  for (int sample_id : sample_ids) {
    // what happens if bad input :(
    filter.erase(filter_ptrs.buffer[sample_id]);
    if (--filter_ptrs.unacked_count == 0) {
      old_buffers.erase(it);
    }
  }
}

void
LearnEngine::LearnList::ack_buffer(buffer_id_t buffer_id) {
  LockType lock(mutex);
  auto it = old_buffers.find(buffer_id);
  // assert(it != old_buffers.end());
  // we assume that this was acked already, and simply return
  if (it == old_buffers.end())
    return;
  FilterPtrs &filter_ptrs = it->second;
  // we optimize for this case (no learning occured since the buffer as sent out
  // and the ack clears out the filter
  if (filter_ptrs.unacked_count == filter.size()) {
    filter.clear();
  } else {  // slow: linear in the number of elements acked
    for (const auto &sample_it : filter_ptrs.buffer) {
      filter.erase(sample_it);
    }
  }
  old_buffers.erase(it);
}

void
LearnEngine::LearnList::reset_state() {
  LockType lock(mutex);
  buffer.clear();
  buffer_id = 0;
  num_samples = 0;
  num_samples_tmp = 0;
  filter.clear();
  old_buffers.clear();
  buffer_tmp.clear();
}

LearnEngine::LearnEngine(int device_id, int cxt_id)
    : device_id(device_id), cxt_id(cxt_id) { }

LearnEngine::LearnList *
LearnEngine::get_learn_list(list_id_t list_id) {
  auto it = learn_lists.find(list_id);
  return it == learn_lists.end() ? nullptr : it->second.get();
}

void
LearnEngine::list_create(list_id_t list_id, size_t max_samples,
                         unsigned int timeout_ms) {
  if (learn_lists.find(list_id) != learn_lists.end()) {
    BMLOG_ERROR("Trying to create learn list with id {} "
                "but a list with the same id already exists", list_id);
        return;
  }
  learn_lists[list_id] = std::unique_ptr<LearnList>(
      new LearnList(list_id, device_id, cxt_id, max_samples, timeout_ms));
}

void
LearnEngine::list_set_learn_writer(
    list_id_t list_id, std::shared_ptr<TransportIface> learn_writer) {
  LearnList *list = get_learn_list(list_id);
  if (!list) {
    BMLOG_ERROR("Trying to set learn writer for invalid learn list id {}",
                list_id);
    return;
  }
  list->set_learn_writer(learn_writer);
}

void
LearnEngine::list_set_learn_cb(list_id_t list_id,
                               const LearnCb &learn_cb, void *cookie) {
  LearnList *list = get_learn_list(list_id);
  if (!list) {
    BMLOG_ERROR("Trying to set learn callback for invalid learn list id {}",
                list_id);
    return;
  }
  list->set_learn_cb(learn_cb, cookie);
}

void
LearnEngine::list_push_back_field(list_id_t list_id,
                                  header_id_t header_id, int field_offset) {
  LearnList *list = get_learn_list(list_id);
  if (!list) {
    BMLOG_ERROR(
        "Trying to push back field ({}, {}) to invalid learn list id {}",
        header_id, field_offset, list_id);
    return;
  }
  list->push_back_field(header_id, field_offset);
}

void
LearnEngine::list_push_back_constant(list_id_t list_id,
                                     const std::string &hexstring) {
  LearnList *list = get_learn_list(list_id);
  if (!list) {
    BMLOG_ERROR("Trying to push back constant {} to invalid learn list id {}",
                hexstring, list_id);
    return;
  }
  list->push_back_constant(hexstring);
}

void
LearnEngine::list_init(list_id_t list_id) {
  LearnList *list = get_learn_list(list_id);
  if (!list) {
    BMLOG_ERROR("Trying to init learn list for invalid id {}", list_id);
    return;
  }
  list->init();
}

LearnEngine::LearnErrorCode
LearnEngine::list_set_timeout(list_id_t list_id, unsigned int timeout_ms) {
  LearnList *list = get_learn_list(list_id);
  if (!list) return INVALID_LIST_ID;
  list->set_timeout(timeout_ms);
  return SUCCESS;
}

LearnEngine::LearnErrorCode
LearnEngine::list_set_max_samples(list_id_t list_id, size_t max_samples) {
  LearnList *list = get_learn_list(list_id);
  if (!list) return INVALID_LIST_ID;
  list->set_max_samples(max_samples);
  return SUCCESS;
}

void
LearnEngine::learn(list_id_t list_id, const Packet &pkt) {
  // TODO(antonin) : event logging
  LearnList *list = get_learn_list(list_id);
  if (!list) {
    BMLOG_ERROR_PKT(pkt, "Trying to learn for invalid list id {}", list_id);
    return;
  }
  list->add_sample(*pkt.get_phv());
}

LearnEngine::LearnErrorCode
LearnEngine::ack(list_id_t list_id, buffer_id_t buffer_id, int sample_id) {
  // TODO(antonin) : event logging
  LearnList *list = get_learn_list(list_id);
  if (!list) return INVALID_LIST_ID;
  list->ack(buffer_id, {sample_id});
  return SUCCESS;
}

LearnEngine::LearnErrorCode
LearnEngine::ack(list_id_t list_id, buffer_id_t buffer_id,
                 const std::vector<int> &sample_ids) {
  // TODO(antonin) : event logging
  LearnList *list = get_learn_list(list_id);
  if (!list) return INVALID_LIST_ID;
  list->ack(buffer_id, sample_ids);
  return SUCCESS;
}

LearnEngine::LearnErrorCode
LearnEngine::ack_buffer(list_id_t list_id, buffer_id_t buffer_id) {
  // TODO(antonin) : event logging
  LearnList *list = get_learn_list(list_id);
  if (!list) return INVALID_LIST_ID;
  list->ack_buffer(buffer_id);
  return SUCCESS;
}

void
LearnEngine::reset_state() {
  for (auto &p : learn_lists)
    p.second->reset_state();
}

}  // namespace bm
