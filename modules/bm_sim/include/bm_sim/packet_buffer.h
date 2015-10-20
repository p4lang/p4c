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

#ifndef _BM_PACKET_BUFFER_H_
#define _BM_PACKET_BUFFER_H_

#include <memory>

#include <cassert>

class PacketBuffer {
public:
  struct state_t {
    char *head;
    size_t data_size;
  };

public:
  PacketBuffer() {}

  PacketBuffer(size_t size)
    : size(size),
      data_size(0),
      buffer(std::unique_ptr<char []>(new char[size])),
      head(buffer.get() + size) {}

  PacketBuffer(size_t size, const char *data, size_t data_size)
    : size(size),
      data_size(0),
      buffer(std::unique_ptr<char []>(new char[size])),
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
  std::unique_ptr<char []> buffer{nullptr};
  char *head{nullptr};
};

#endif
