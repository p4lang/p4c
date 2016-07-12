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

//! @file packet.h

#ifndef BM_BM_SIM_PACKET_H_
#define BM_BM_SIM_PACKET_H_

#include <array>
#include <memory>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <atomic>
#include <algorithm>  // for std::min
#include <limits>

#include <cassert>

#include "packet_buffer.h"
#include "phv_source.h"
#include "phv.h"

namespace bm {

//! Integral type used to identify a given data packet
typedef uint64_t packet_id_t;

//! Integral type used to distinguish between different clones of a packet
typedef uint64_t copy_id_t;

class CopyIdGenerator {
 private:
  static constexpr size_t W = 4096;
  struct Cntr {
    // if default member initializer used, then g++-5 complains for some reason,
    // so I had to provide an explicit constructor
    // clang complains if 'noexcept' is missing
    Cntr() noexcept : max(0), num(0) { }
    uint16_t max;
    uint16_t num;
  };

 public:
  copy_id_t add_one(packet_id_t packet_id);

  void remove_one(packet_id_t packet_id);

  copy_id_t get(packet_id_t packet_id);

  void reset(packet_id_t packet_id);

 private:
  std::array<std::atomic<Cntr>, W> arr{{}};
};


//! One of the most fundamental classes of bmv2. Used to represent a data packet
//! while it traverses the switch. Even when a packet is parsed / deparsed
//! mutliple times, the same Packet instance is used to represent it. If a
//! Packet needs to be duplicated (for cloning, multicast, ...), a new Packet
//! instance is created, using one of the available clone functions.
//!
//! This class is `final`, so if the Packet class needs to be extended in your
//! target, you will need to use composition, not inheritance. We weigh the pros
//! and cons, and in the end we decided that preventing the Packet class from
//! being subclassed was safer. It does come at a cost though (in action
//! primitives, you can access the Packet instance, but not your target-specific
//! packet class).
//!
//! Note that in your target implementation, Packet instances should be created
//! using SwitchWContexts::new_packet() / SwitchWContexts::new_packet_ptr() or
//! Switch::new_packet() / Switch::new_packet_ptr() depending on your target
//! design.
class Packet final {
  friend class SwitchWContexts;
  friend class Switch;

 public:
  typedef std::chrono::system_clock clock;

  typedef PacketBuffer::state_t buffer_state_t;

  //! Number of general purpose registers per packet
  static constexpr size_t nb_registers = 2u;

  ~Packet();

  //! Obtain the packet_id. The packet_id is the one assigned by the target when
  //! the packet was instantiated. We recommend using a counter initialized to
  //! `0` and incremented every time a new packet is received "on the
  //! wire". Packet instances obtained by cloning a given packet receives the
  //! same packet_id as that packet. In other words, this packet_id identifies
  //! all Packet instances produced by the switch in response to the same
  //! incoming data packet. The clones can be differentiated using the copy_id.
  packet_id_t get_packet_id() const { return packet_id; }

  //! Get the egress_port of this Packet. The egress_port needs to be set by the
  //! target using set_ingress_port().
  int get_egress_port() const { return egress_port; }
  //! Get the ingress_port of the packet.
  int get_ingress_port() const { return ingress_port; }

  //! Set the egress_port of the packet, which is the target responsibility.
  void set_egress_port(int port) { egress_port = port; }
  void set_ingress_port(int port) { ingress_port = port; }

  //! Obtains the copy_id of a packet, used to differentiate all the clones
  //! generated from the same incoming data packet. See get_packet_id() for more
  //! information.
  copy_id_t get_copy_id() const { return copy_id; }

  //! Get a unique string id for this packet.
  //! It is simply `packet_id + "." + copy_id`.
  const std::string get_unique_id() const {
    return std::to_string(packet_id) + "." + std::to_string(copy_id);
  }

  void set_copy_id(copy_id_t id) { copy_id = id; }

  //! Get the ingress_length for the packet, which is set when instantiating the
  //! packet. As the name indicates, this is the original length of the
  //! packet. This is not updated as headers are marked valid / invalid and the
  //! length of the outgoing packet may be different.
  int get_ingress_length() const { return ingress_length; }

  void set_payload_size(size_t size) { payload_size = size; }

  size_t get_payload_size() const { return payload_size; }

  //! Returns the amount of packet data in the buffer (in bytes). This function
  //! will notably be called after deparsing, when the target is ready to
  //! transmit the packet.
  size_t get_data_size() const {
    return std::min(buffer.get_data_size(), truncated_length);
  }

  //! Returns a pointer to the packet data. Just after instantiating the packet,
  //! this will point to all the received packet data. After parsing, this will
  //! just be the packet payload. After deparsing, this will be data which needs
  //! to be sent out.
  char *data() { return buffer.start(); }

  //! @copydoc data
  const char *data() const { return buffer.start(); }

  //! Saves the current state of the packet data buffer. Even if the buffer
  //! state is then modified, the state can be restored by providing the return
  //! value of this function to restore_buffer_state().
  const buffer_state_t save_buffer_state() const {
    return buffer.save_state();
  }

  //! See save_buffer_state()
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

  //! Truncate the packet to the given \p length. The truncation will only be
  //! enforced when calling get_data_size(). When calling truncate multiple
  //! times, the smallest provided \p length value will be used.
  void truncate(size_t length) {
    truncated_length = std::min(length, truncated_length);
  }

  char *prepend(size_t bytes) { return buffer.push(bytes); }

  char *remove(size_t bytes) {
    assert(buffer.get_data_size() >= payload_size + bytes);
    return buffer.pop(bytes);
  }

  //! Get a 64-bit hash of the incoming packet data.
  uint64_t get_signature() const {
    return signature;
  }

  const PacketBuffer &get_packet_buffer() const { return buffer; }

  uint64_t get_ingress_ts_ms() const { return ingress_ts_ms; }

  // TODO(antonin): use references instead?
  //! Get a pointer to the packet's phv.
  PHV *get_phv() { return phv.get(); }
  //! @copydoc get_phv
  const PHV *get_phv() const { return phv.get(); }

  //! Write to general purpose register at index \p idx
  void set_register(size_t idx, uint64_t v) { registers.at(idx) = v; }
  //! Read general purpose register at index \p idx
  uint64_t get_register(size_t idx) { return registers.at(idx); }

  //! Mark the packet for exit by setting an exit flag. If this is called from
  //! an action, the current pipeline will be interrupted as soon as the current
  //! table is done processing the packet. This effectively skips the rest of
  //! the pipeline. Do not forget to call reset_exit() at the end of the
  //! pipeline, if you intend the packet to go through another pipeline
  //! afterwards; otherwise this other pipeline will just be skipped as well. If
  //! this method is called from outside of an action, the flag will take effect
  //! at the next pipeline, which will be skipped entirely (not a single table
  //! will be applied).
  void mark_for_exit() { flags |= 1 << FLAGS_EXIT; }
  //! Clear the exit flag, which may have been set by an earlier call to
  //! mark_for_exit().
  void reset_exit() { flags &= ~(1 << FLAGS_EXIT); }
  //! Returns true iff the packet is marked for exit (i.e. mark_for_exit() was
  //! called on the packet).
  bool is_marked_for_exit() const { return flags & (1 << FLAGS_EXIT); }

  //! Changes the context of the packet. You will only need to call this
  //! function if you target switch leverages the Context class and if your
  //! Packet instance changes contexts during its lifetime. This is needed
  //! because one can configure different contexts to support different header
  //! types, and the PHV templates for different contexts will therefore be
  //! different. When changing contexts, it is up to the user to 1) deparse the
  //! Packet in the old Context 2) call change_context 3) re-parse the Packet in
  //! the new Context. For example:
  //! @code
  //! context1->get_deparser("deparser1")->deparse(pkt);
  //! pkt->change_context(2);
  //! context2->get_parser("parser1")->parse(pkt);
  //! @endcode
  //! if `new_cxt == cxt_id`, then this is a no-op,
  //! otherwise, release the old PHV and replace it with a new one, compatible
  //! with the new context
  void change_context(size_t new_cxt);

  //! Returns the id of the Context this packet currently belongs to
  size_t get_context() const { return cxt_id; }

  // the *_ptr function are just here for convenience, the same can be achieved
  // by the client by constructing a unique_ptr

  //! Clone the current packet, along with its PHV. The value of all the fields
  //! (metadata and regular) will remain the same int the clone.
  Packet clone_with_phv() const;
  //! @copydoc clone_with_phv
  std::unique_ptr<Packet> clone_with_phv_ptr() const;

  //! Clone the current packet, along with regular headers, but resets all the
  //! metadata fields (i.e. sets them to `0`) in the clone.
  Packet clone_with_phv_reset_metadata() const;
  //! @copydoc clone_with_phv_reset_metadata
  std::unique_ptr<Packet> clone_with_phv_reset_metadata_ptr() const;

  //! Clone the current packet, without the PHV. The value of the fields in the
  //! clone will be undefined and should not be accessed before setting it
  //! first.
  Packet clone_no_phv() const;
  //! @copydoc clone_no_phv
  std::unique_ptr<Packet> clone_no_phv_ptr() const;

  //! Same as clone_no_phv(), but also changes the context id for the clone.
  //! See change_context() for more information on how a Packet instance belongs
  //! to a specific Context
  Packet clone_choose_context(size_t new_cxt) const;
  //! @copydoc clone_choose_context
  std::unique_ptr<Packet> clone_choose_context_ptr(size_t new_cxt) const;

  //! Deleted copy constructor
  Packet(const Packet &other) = delete;
  //! Deleted copy assignment operator
  Packet &operator=(const Packet &other) = delete;

  //! Move constructor
  Packet(Packet &&other) noexcept;
  //! Move assignment operator
  Packet &operator=(Packet &&other) noexcept;

  // for tests
  // TODO(antonin): find a better solution, no-one is supposed to use these
  static Packet make_new(PHVSourceIface *phv_source);
  // cpplint false positive
  // NOLINTNEXTLINE(whitespace/operators)
  static Packet make_new(int ingress_length, PacketBuffer &&buffer,
                         PHVSourceIface *phv_source);
  static Packet make_new(size_t cxt, int ingress_port, packet_id_t id,
                         copy_id_t copy_id, int ingress_length,
                         // cpplint false positive
                         // NOLINTNEXTLINE(whitespace/operators)
                         PacketBuffer &&buffer, PHVSourceIface *phv_source);

 private:
  Packet(size_t cxt, int ingress_port, packet_id_t id, copy_id_t copy_id,
         int ingress_length, PacketBuffer &&buffer, PHVSourceIface *phv_source);

  void update_signature(uint64_t seed = 0);
  void set_ingress_ts();

  size_t cxt_id{0};
  int ingress_port{-1};
  int egress_port{-1};
  packet_id_t packet_id{0};
  copy_id_t copy_id{0};
  int ingress_length{0};

  enum PacketFlags {
    FLAGS_EXIT = 0
  };
  int flags{0};

  uint64_t signature{0};

  PacketBuffer buffer{};

  size_t payload_size{0};

  size_t truncated_length{std::numeric_limits<size_t>::max()};

  clock::time_point ingress_ts{};
  uint64_t ingress_ts_ms{};

  std::unique_ptr<PHV> phv{nullptr};

  PHVSourceIface *phv_source{nullptr};

  // General purpose registers available to a target, they can be written with
  // Packet::set_register and read with Packet::get_register
  std::array<uint64_t, nb_registers> registers;

 private:
  static CopyIdGenerator *copy_id_gen;
};

}  // namespace bm

#endif  // BM_BM_SIM_PACKET_H_
