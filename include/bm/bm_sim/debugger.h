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

#ifndef BM_BM_SIM_DEBUGGER_H_
#define BM_BM_SIM_DEBUGGER_H_

#include <limits>
#include <string>

// #define BMDEBUG_ON

namespace bm {

/* This whole code if for a proof of concept and is temporary */

// forward declaration
class DebuggerIface;

// TODO(antonin)
// temprorary: experimenting with debugger
#define DBG_CTR_PARSER (1 << 24)
#define DBG_CTR_PARSE_STATE (2 << 24)
#define DBG_CTR_CONTROL (3 << 24)
#define DBG_CTR_TABLE (4 << 24)
#define DBG_CTR_CONDITION (5 << 24)
#define DBG_CTR_ACTION (6 << 24)
#define DBG_CTR_DEPARSER (7 << 24)

#define DBG_CTR_EXIT(x) (x | (0x80 << 24))

class Debugger {
 public:
  static constexpr uint64_t FIELD_COND =
      std::numeric_limits<uint64_t>::max() - 1;

  static constexpr uint64_t FIELD_ACTION =
      std::numeric_limits<uint64_t>::max() - 2;

  struct PacketId {
    uint64_t packet_id;
    uint64_t copy_id;

    bool operator==(const PacketId& other) const {
      return packet_id == other.packet_id && copy_id == other.copy_id;
    }

    bool operator!=(const PacketId& other) const {
      return !(*this == other);
    }

    static PacketId make(uint64_t packet_id, uint64_t copy_id) {
      return {packet_id, copy_id};
    }
  };

  static void init_debugger(const std::string &addr);

  static DebuggerIface *get() {
    return debugger;
  }

  // returns an empty string if debugger wasn't initialized
  static std::string get_addr();

  static DebuggerIface *debugger;

  static constexpr PacketId dummy_PacketId{0u, 0u};

 private:
  static bool is_init;
};

class DebuggerIface {
 public:
  typedef Debugger::PacketId PacketId;

  void notify_update(const PacketId &packet_id,
                     uint64_t id, const char *bytes, int nbits) {
    notify_update_(packet_id, id, bytes, nbits);
  }

  void notify_update(const PacketId &packet_id,
                     uint64_t id, uint32_t v) {
    notify_update_(packet_id, id, v);
  }

  void notify_ctr(const PacketId &packet_id, uint32_t ctr) {
    notify_ctr_(packet_id, ctr);
  }

  void packet_in(const PacketId &packet_id, int port) {
    packet_in_(packet_id, port);
  }

  void packet_out(const PacketId &packet_id, int port) {
    packet_out_(packet_id, port);
  }

  void config_change() {
    config_change_();
  }

  std::string get_addr() const {
    return get_addr_();
  }

 protected:
  ~DebuggerIface() { }

 private:
  virtual void notify_update_(const PacketId &packet_id, uint64_t id,
                              const char *bytes, int nbits) = 0;

  virtual void notify_update_(const PacketId &packet_id, uint64_t id,
                              uint32_t v) = 0;

  virtual void notify_ctr_(const PacketId &packet_id, uint32_t ctr) = 0;

  virtual void packet_in_(const PacketId &packet_id, int port) = 0;

  virtual void packet_out_(const PacketId &packet_id, int port) = 0;

  virtual void config_change_() = 0;

  virtual std::string get_addr_() const = 0;
};

#ifdef BMDEBUG_ON
#define DEBUGGER_NOTIFY_UPDATE(packet_id, id, bytes, nbits) \
  Debugger::get()->notify_update(packet_id, id, bytes, nbits);
#define DEBUGGER_NOTIFY_UPDATE_V(packet_id, id, v) \
  Debugger::get()->notify_update(packet_id, id, v);
#define DEBUGGER_NOTIFY_CTR(packet_id, ctr) \
  Debugger::get()->notify_ctr(packet_id, ctr);
#define DEBUGGER_PACKET_IN(packet_id, port) \
  Debugger::get()->packet_in(packet_id, port);
#define DEBUGGER_PACKET_OUT(packet_id, port) \
  Debugger::get()->packet_out(packet_id, port);
#else
#define DEBUGGER_NOTIFY_UPDATE(packet_id, id, bytes, nbits)
#define DEBUGGER_NOTIFY_UPDATE_V(packet_id, id, v)
#define DEBUGGER_NOTIFY_CTR(packet_id, ctr)
#define DEBUGGER_PACKET_IN(packet_id, port)
#define DEBUGGER_PACKET_OUT(packet_id, port)
#endif

}  // namespace bm

#endif  // BM_BM_SIM_DEBUGGER_H_
