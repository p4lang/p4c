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

#ifndef BM_SIM_INCLUDE_BM_SIM_PACKET_H_
#define BM_SIM_INCLUDE_BM_SIM_PACKET_H_

#include <array>
#include <memory>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <atomic>

#include <cassert>

#include "packet_buffer.h"
#include "phv.h"

typedef uint64_t packet_id_t;
typedef uint64_t copy_id_t;

class CopyIdGenerator {
 private:
  static constexpr size_t W = 4096;

 public:
  copy_id_t add_one(packet_id_t packet_id) {
    int idx = static_cast<int>(packet_id % W);
    return ++arr[idx];
  }

  void remove_one(packet_id_t packet_id) {
    int idx = static_cast<int>(packet_id % W);
    --arr[idx];
  }

  copy_id_t get(packet_id_t packet_id) {
    int idx = static_cast<int>(packet_id % W);
    return arr[idx];
  }

  void reset(packet_id_t packet_id) {
    int idx = static_cast<int>(packet_id % W);
    arr[idx] = 0;
  }

 private:
  std::array<std::atomic<int>, W> arr{{}};
};

class Packet {
 public:
  typedef std::chrono::system_clock clock;

  typedef PacketBuffer::state_t buffer_state_t;

  virtual ~Packet();

  packet_id_t get_packet_id() const { return packet_id; }

  int get_egress_port() const { return egress_port; }

  int get_ingress_port() const { return ingress_port; }

  void set_egress_port(int port) { egress_port = port; }
  void set_ingress_port(int port) { ingress_port = port; }

  copy_id_t get_copy_id() const { return copy_id; }

  const std::string get_unique_id() const {
    return std::to_string(packet_id) + "." + std::to_string(copy_id);
  }

  void set_copy_id(copy_id_t id) { copy_id = id; }

  int get_ingress_length() const { return ingress_length; }

  void set_payload_size(size_t size) { payload_size = size; }

  size_t get_payload_size() const { return payload_size; }

  size_t get_data_size() const { return buffer.get_data_size(); }

  char *data() { return buffer.start(); }

  const char *data() const { return buffer.start(); }

  const buffer_state_t save_buffer_state() const {
    return buffer.save_state();
  }

  void restore_buffer_state(const buffer_state_t &state) {
    buffer.restore_state(state);
  }

  char *payload() {
    assert(payload_size > 0);
    return buffer.end() - payload_size;
  }

  const char *payload() const {
    assert(payload_size > 0);
    return buffer.end() - payload_size;
  }

  char *prepend(size_t bytes) { return buffer.push(bytes); }

  char *remove(size_t bytes) {
    assert(buffer.get_data_size() >= payload_size + bytes);
    return buffer.pop(bytes);
  }

  uint64_t get_signature() const {
    return signature;
  }
  const PacketBuffer &get_packet_buffer() const { return buffer; }

  uint64_t get_ingress_ts_ms() const { return ingress_ts_ms; }

  // TODO(antonin): use references instead?
  PHV *get_phv() { return phv.get(); }
  const PHV *get_phv() const { return phv.get(); }

  Packet clone() const;
  Packet clone_and_reset_metadata() const;
  Packet clone_no_phv() const;

  Packet(const Packet &other) = delete;
  Packet &operator=(const Packet &other) = delete;

  Packet(Packet &&other) noexcept;
  Packet &operator=(Packet &&other) noexcept;

  // cpplint false positive for rvalue references
  static Packet make_new(int ingress_port, packet_id_t id, int ingress_length,
  // NOLINTNEXTLINE(whitespace/operators)
                         PacketBuffer &&buffer);
  static Packet make_new(int ingress_port, packet_id_t id, int ingress_length,
  // NOLINTNEXTLINE(whitespace/operators)
                         PacketBuffer &&buffer,
                         const PHV &src_phv);
  // for testing
  static Packet make_new(packet_id_t id = 0);

 protected:
  Packet(int ingress_port, packet_id_t id, copy_id_t copy_id,
         int ingress_length, PacketBuffer &&buffer);

  Packet(int ingress_port, packet_id_t id, copy_id_t copy_id,
         int ingress_length, PacketBuffer &&buffer, const PHV &src_phv);

  explicit Packet(packet_id_t id);

  void update_signature(uint64_t seed = 0);
  void set_ingress_ts();

  int ingress_port{-1};
  int egress_port{-1};
  packet_id_t packet_id{0};
  copy_id_t copy_id{0};
  int ingress_length{0};

  uint64_t signature{0};

  PacketBuffer buffer{};

  size_t payload_size{0};

  clock::time_point ingress_ts{};
  uint64_t ingress_ts_ms{};

  std::unique_ptr<PHV> phv{nullptr};

 private:
  class PHVPool {
   public:
    explicit PHVPool(const PHVFactory &phv_factory);
    std::unique_ptr<PHV> get();
    void release(std::unique_ptr<PHV> phv);

   private:
    mutable std::mutex mutex{};
    std::vector<std::unique_ptr<PHV> > phvs{};
    const PHVFactory &phv_factory;
  };

 public:
  static void set_phv_factory(const PHVFactory &phv_factory);
  static void unset_phv_factory();
  static void swap_phv_factory(const PHVFactory &phv_factory);

 private:
  // static variable
  // Google style guidelines stipulate that we have to use a raw pointer and
  // leak the memory
  static PHVPool *phv_pool;

  static CopyIdGenerator *copy_id_gen;
};

#endif  // BM_SIM_INCLUDE_BM_SIM_PACKET_H_
