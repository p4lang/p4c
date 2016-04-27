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

//! @file packet_buffer.h

#ifndef BM_BM_SIM_PACKET_BUFFER_H_
#define BM_BM_SIM_PACKET_BUFFER_H_

#include <memory>
#include <algorithm>  // for std::copy

#include <cassert>

namespace bm {

//! This acts as a recipient for the packet data. A PacketBuffer instance will
//! belong to a Packet instance and the same PacketBuffer is used to hold 1) the
//! unparsed packet when the packet is first received 2) the packet payload
//! (i.e. data that was not extracted) while the packet is being processed by
//! the match-action tables 3) the deparsed packet ready to be transmitted.
//! As a target switch programmer, you will only encounter PacketBuffer when
//! creating a new Packet instance:
//! @code
//! // A new packet was received on port_num, its ingress length is len, we
//! // assign pkt_id as its packet id (and increment the counter for the next
//! // incoming packet). We provide a PacketBuffer with capacity 2048 and
//! // initialize it with the incoming packet data.
//! auto packet = new_packet_ptr(port_num, pkt_id++, len,
//!                              PacketBuffer(2048, buffer, len));
//! @endcode
class PacketBuffer {
 public:
  struct state_t {
    char *head;
    size_t data_size;
  };

 public:
  PacketBuffer() {}

  explicit PacketBuffer(size_t size)
    : size(size),
      data_size(0),
      buffer(std::unique_ptr<char[]>(new char[size])),
      head(buffer.get() + size) {}

  //! Construct a PacketBuffer instance with capacity \p size, and copy the
  //! bytes `[data; data + data_size)` to the new buffer. The \p data is
  //! actually copied to the end of the buffer. This is because while we never
  //! need to append data at the end of a packet, the header size of the
  //! outgoing packet may be larger than the header size of the incoming
  //! packet. By copying the initial \p data at the end of the buffer, we
  //! preserve space at the beginning of the buffer for potential new
  //! headers.
  //!
  //! The capacity \p size of the PacketBuffer needs to be at least as big as \p
  //! data_size. If new headers are going to be added to the pakcet during
  //! processing, \p size needs to be at least as big as the size of the
  //! outgoing packet.
  PacketBuffer(size_t size, const char *data, size_t data_size)
    : size(size),
      data_size(0),
      buffer(std::unique_ptr<char[]>(new char[size])),
      head(buffer.get() + size) {
    std::copy(data, data + data_size, push(data_size));
  }

  char *start() const { return head; }

  char *end() const { return buffer.get() + size; }

  char *push(size_t bytes) {
    assert(data_size + bytes <= size);
    data_size += bytes;
    head -= bytes;
    return head;
  }

  char *pop(size_t bytes) {
    assert(bytes <= data_size);
    data_size -= bytes;
    head += bytes;
    return head;
  }

  const state_t save_state() const {
    return {head, data_size};
  }

  void restore_state(const state_t &state) {
    head = state.head;
    data_size = state.data_size;
  }

  size_t get_data_size() const { return data_size; }

  PacketBuffer clone(size_t end_bytes) const {
    assert(end_bytes <= data_size);
    PacketBuffer pb(size);
    std::copy(end() - end_bytes, end(), pb.push(end_bytes));
    return pb;
  }

  PacketBuffer(const PacketBuffer &other) = delete;
  PacketBuffer &operator=(const PacketBuffer &other) = delete;

  PacketBuffer(PacketBuffer &&other) /*noexcept*/ = default;
  PacketBuffer &operator=(PacketBuffer &&other) /*noexcept*/ = default;

 private:
  size_t size{0};
  size_t data_size{0};
  std::unique_ptr<char[]> buffer{nullptr};
  char *head{nullptr};
};

}  // namespace bm

#endif  // BM_BM_SIM_PACKET_BUFFER_H_
