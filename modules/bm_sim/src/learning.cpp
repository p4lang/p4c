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

#include <cassert>

#include "bm_sim/learning.h"

template <typename Transport>
LearnWriterImpl<Transport>::LearnWriterImpl(const std::string &transport_name)
  : transport_instance(std::unique_ptr<Transport>(new Transport()))
{
  // should not be doing this in the constructor ?
  transport_instance->open(transport_name);
}

template <typename Transport>
int LearnWriterImpl<Transport>::send(const char *buffer, size_t len) const 
{
  return transport_instance->send(buffer, len);
}

template <typename Transport>
int LearnWriterImpl<Transport>::send_msgs(
    const std::initializer_list<TransportIface::MsgBuf> &msgs
) const
{
  return transport_instance->send_msgs(msgs);
}

// explicit template instantiation
template class LearnWriterImpl<TransportNanomsg>;

void LearnEngine::LearnSampleBuilder::push_back_constant(
    const ByteContainer &constant
)
{
  unsigned int offset = constants.size();
  constants.push_back(constant);
  LearnSampleEntry entry;
  entry.tag = LearnSampleEntry::CONSTANT;
  entry.constant.offset = offset;
  entries.push_back(entry);
}

void LearnEngine::LearnSampleBuilder::push_back_field(
    header_id_t header_id, int field_offset
)
{
  LearnSampleEntry entry;
  entry.tag = LearnSampleEntry::FIELD;
  entry.field.header = header_id;
  entry.field.offset = field_offset;
  entries.push_back(entry);
}

void LearnEngine::LearnSampleBuilder::operator()(
    const PHV &phv, ByteContainer *sample
) const
{
  for(const LearnSampleEntry &entry : entries) {
    const ByteContainer *bytes = nullptr;
    switch(entry.tag) {
    case LearnSampleEntry::FIELD:
      bytes = &phv.get_field(entry.field.header, entry.field.offset).get_bytes();
      break;
    case LearnSampleEntry::CONSTANT:
      bytes = &constants[entry.constant.offset];
      break;
    }
    sample->append(*bytes);
    // buffer.insert(buffer.end(), bytes->begin(), bytes->end());
  }
}

LearnEngine::LearnList::LearnList(
    list_id_t list_id, size_t max_samples, unsigned int timeout
) : list_id(list_id), max_samples(max_samples),
    timeout(timeout), with_timeout(timeout > 0) { }

void LearnEngine::LearnList::init()
{
  transmit_thread = std::thread(&LearnList::buffer_transmit_loop, this);
  // transmit_thread.detach();
}

LearnEngine::LearnList::~LearnList()
{
  {
    std::unique_lock<std::mutex> lock(mutex);
    stop_transmit_thread = true;
  }
  b_can_send.notify_all();
  transmit_thread.join();
}

void LearnEngine::LearnList::set_learn_writer(
    std::shared_ptr<LearnWriter> learn_writer
)
{
  std::unique_lock<std::mutex> lock(mutex);
  while(writer_busy) {
    can_change_writer.wait(lock);  
  }
  learn_mode = LearnMode::WRITER;
  writer = learn_writer;
}

void LearnEngine::LearnList::set_learn_cb(
    const LearnCb &learn_cb, void *cookie
)
{
  std::unique_lock<std::mutex> lock(mutex);
  while(writer_busy) {
    can_change_writer.wait(lock);  
  }
  learn_mode = LearnMode::CB;
  cb_fn = learn_cb;
  cb_cookie = cookie;
}

void LearnEngine::LearnList::push_back_field(
    header_id_t header_id, int field_offset
)
{
  builder.push_back_field(header_id, field_offset);
}

void LearnEngine::LearnList::push_back_constant(const std::string &hexstring)
{
  builder.push_back_constant(ByteContainer(hexstring));
}

void LearnEngine::LearnList::swap_buffers() {
  buffer_tmp.swap(buffer);
  num_samples = 0;
  buffer_id++;
}

void LearnEngine::LearnList::add_sample(const PHV &phv)
{
  static thread_local ByteContainer sample;
  sample.clear();
  builder(phv, &sample);

  std::unique_lock<std::mutex> lock(mutex);

  const auto it = filter.find(sample);
  if(it != filter.end()) return;

  buffer.insert(buffer.end(), sample.begin(), sample.end());
  num_samples++;
  auto filter_it = filter.insert(filter.end(), std::move(sample));
  FilterPtrs &filter_ptrs = old_buffers[buffer_id];
  filter_ptrs.unacked_count++;
  filter_ptrs.buffer.push_back(filter_it);

  if(num_samples == 1 && max_samples > 1) {
    buffer_started = clock::now(); 
    b_can_send.notify_one(); // wake transmit thread to update cond var wait time
  }
  else if(num_samples >= max_samples) {
    while(buffer_tmp.size() != 0) {
      b_can_swap.wait(lock);
    }
    swap_buffers();

    b_can_send.notify_one();
  }
}

void LearnEngine::LearnList::buffer_transmit()
{
  milliseconds time_to_sleep;
  size_t num_samples_to_send;
  std::unique_lock<std::mutex> lock(mutex);
  clock::time_point now = clock::now();
  while(!stop_transmit_thread &&
	buffer_tmp.size() == 0 &&
	(!with_timeout || num_samples == 0 || now < (buffer_started + timeout))) {
    if(with_timeout && num_samples > 0) {
      b_can_send.wait_until(lock, buffer_started + timeout);
    }
    else {
      b_can_send.wait(lock);
    }
    now = clock::now();
  }

  if(stop_transmit_thread) return;

  if(buffer_tmp.size() == 0) { // timeout -> we need to swap here
    num_samples_to_send = num_samples;
    swap_buffers();
  }
  else { // buffer full -> swapping was already done
    num_samples_to_send = max_samples;
  }

  last_sent = now;
  writer_busy = true;
  
  lock.unlock();

  msg_hdr_t msg_hdr = {0, list_id, buffer_id - 1,
		       (unsigned int) num_samples_to_send};

  if(learn_mode == LearnMode::WRITER) {
    // do not forget the -1 !!!
    // swap_buffers() is in charge of incrementing the count
    TransportIface::MsgBuf buf_hdr = {(char *) &msg_hdr, sizeof(msg_hdr)};
    TransportIface::MsgBuf buf_samples = {buffer_tmp.data(),
					  (unsigned int) buffer_tmp.size()};
    
    writer->send_msgs({buf_hdr, buf_samples}); // no lock for I/O
  }
  else if(learn_mode == LearnMode::CB) {
    std::unique_ptr<char []> buf(new char[buffer_tmp.size()]);
    std::copy(buffer_tmp.begin(), buffer_tmp.end(), &buf[0]);
    cb_fn(msg_hdr, buffer_tmp.size(), std::move(buf), cb_cookie); 
  }
  else {
    assert(learn_mode == LearnMode::NONE);
  }

  lock.lock();

  writer_busy = false;
  can_change_writer.notify_all();

  buffer_tmp.clear();

  b_can_swap.notify_one();
}

void LearnEngine::LearnList::buffer_transmit_loop()
{
  while(1) {
    buffer_transmit();
    if(stop_transmit_thread) return;
  }
}

void LearnEngine::LearnList::ack(
    buffer_id_t buffer_id, const std::vector<int> &sample_ids
)
{
  std::unique_lock<std::mutex> lock(mutex);
  auto it = old_buffers.find(buffer_id);
  // assert(it != old_buffers.end());
  // we assume that this was acked already, and simply return
  if(it == old_buffers.end())
    return;
  FilterPtrs &filter_ptrs = it->second;
  for (int sample_id : sample_ids) {
    filter.erase(filter_ptrs.buffer[sample_id]); // what happens if bad input :(
    if(--filter_ptrs.unacked_count == 0) {
      old_buffers.erase(it);
    }
  }
}

void LearnEngine::LearnList::ack_buffer(buffer_id_t buffer_id)
{
  std::unique_lock<std::mutex> lock(mutex);
  auto it = old_buffers.find(buffer_id);
  // assert(it != old_buffers.end());
  // we assume that this was acked already, and simply return
  if(it == old_buffers.end())
    return;
  FilterPtrs &filter_ptrs = it->second;
  // we optimize for this case (no learning occured since the buffer as sent out
  // and the ack clears out the filter
  if(filter_ptrs.unacked_count == filter.size()) {
    filter.clear();
  }
  else { // slow: linear in the number of elements acked
    for(const auto &sample_it : filter_ptrs.buffer) {
      filter.erase(sample_it);
    }
  }
  old_buffers.erase(it);
}

void LearnEngine::LearnList::reset_state()
{
  std::unique_lock<std::mutex> lock(mutex);
  // TODO: need to make this robust, what if an unexpected ack comes later?
  buffer.clear();
  buffer_id = 0;
  num_samples = 0;
  filter.clear();
  old_buffers.clear();
  buffer_tmp.clear();
}

void LearnEngine::list_create(
    list_id_t list_id, size_t max_samples, unsigned int timeout_ms
)
{
  assert(learn_lists.find(list_id) == learn_lists.end());
  learn_lists[list_id] =
    std::unique_ptr<LearnList>(new LearnList(list_id, max_samples, timeout_ms));
}

void LearnEngine::list_set_learn_writer(
    list_id_t list_id, std::shared_ptr<LearnWriter> learn_writer
)
{
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->set_learn_writer(learn_writer);
}

void LearnEngine::list_set_learn_cb(
    list_id_t list_id, const LearnCb &learn_cb, void *cookie
)
{
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->set_learn_cb(learn_cb, cookie);
}

void LearnEngine::list_push_back_field(
    list_id_t list_id, header_id_t header_id, int field_offset
) {
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->push_back_field(header_id, field_offset);
}

void LearnEngine::list_push_back_constant(
    list_id_t list_id, const std::string &hexstring
) {
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->push_back_constant(hexstring);
}

void LearnEngine::list_init(list_id_t list_id)
{
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->init();
}

void LearnEngine::learn(list_id_t list_id, const Packet &pkt) {
  // TODO : event logging
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->add_sample(*pkt.get_phv());
}

void LearnEngine::ack(list_id_t list_id, buffer_id_t buffer_id, int sample_id) {
  // TODO : event logging
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->ack(buffer_id, {sample_id});
}

void LearnEngine::ack(list_id_t list_id, buffer_id_t buffer_id,
		      const std::vector<int> &sample_ids) {
  // TODO : event logging
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->ack(buffer_id, sample_ids);
}

void LearnEngine::ack_buffer(list_id_t list_id, buffer_id_t buffer_id) {
  // TODO : event logging
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->ack_buffer(buffer_id);
}

void LearnEngine::reset_state() {
  for(auto &p : learn_lists)
    p.second->reset_state();
}
