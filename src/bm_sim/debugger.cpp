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

#include <bm/bm_sim/debugger.h>
#include <bm/bm_sim/nn.h>

// temporary deps?
#include <bm/bm_sim/bytecontainer.h>
#include <bm/bm_sim/logger.h>

#include <boost/functional/hash.hpp>
#include <nanomsg/reqrep.h>

#include <chrono>
#include <memory>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <string>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace bm {

static_assert(std::is_pod<Debugger::PacketId>::value,
              "Debugger::PacketId must be a POD type.");

constexpr Debugger::PacketId Debugger::dummy_PacketId;

typedef Debugger::PacketId PacketId;

class DebuggerDummy final : public DebuggerIface {
 private:
  void notify_update_(const PacketId &packet_id,
                      uint64_t id, const char *bytes, int nbits) override {
    (void) packet_id;
    (void) id;
    (void) bytes;
    (void) nbits;
  }

  void notify_update_(const PacketId &packet_id,
                      uint64_t id, uint32_t v) override {
    (void) packet_id;
    (void) id;
    (void) v;
  }

  void notify_ctr_(const PacketId &packet_id, uint32_t ctr) override {
    (void) packet_id;
    (void) ctr;
  }

  void packet_in_(const PacketId &packet_id, int port) override {
    (void) packet_id;
    (void) port;
  }

  void packet_out_(const PacketId &packet_id, int port) override {
    (void) packet_id;
    (void) port;
  }

  void config_change_() override { }

  std::string get_addr_() const override {
    return "";
  }
};

class DebuggerNN final : public DebuggerIface {
 public:
  explicit DebuggerNN(const std::string &addr);

  ~DebuggerNN();

  void open();

 private:
  typedef std::unique_lock<std::mutex> UniqueLock;

  void notify_update_(const PacketId &packet_id,
                      uint64_t id, const char *bytes, int nbits) override;
  void notify_update_(const PacketId &packet_id,
                      uint64_t id, uint32_t v) override;

  void notify_ctr_(const PacketId &packet_id, uint32_t ctr) override;

  void packet_in_(const PacketId &packet_id, int port) override;

  void packet_out_(const PacketId &packet_id, int port) override;

  void config_change_() override;

  std::string get_addr_() const override {
    return addr;
  }

  void reset_state();

  void notify_update_generic(const PacketId &packet_id, uint64_t id,
                             const char *bytes, int nbits);

  // TODO(antonin): find better solution
  // NOLINTNEXTLINE(runtime/references)
  void wait_for_continue(UniqueLock &lock) const;
  // NOLINTNEXTLINE(runtime/references)
  void wait_for_resume_packet_in(UniqueLock &lock) const;

  void request_in();
  void request_dispatch(const int msg_type, const char *buffer);


  void req_continue(const char *data);
  void req_next(const char *data);
  void req_get_value(const char *data);
  void req_get_backtrace(const char *data);
  void req_break_packet_in(const char *data);
  void req_remove_packet_in(const char *data);
  void req_stop_packet_in(const char *data);
  void req_resume_packet_in(const char *data);
  void req_filter_notifications(const char *data);
  void req_set_watchpoint(const char *data);
  void req_unset_watchpoint(const char *data);
  void req_reset(const char *data);
  void req_attach(const char *data);
  void req_detach(const char *data);

  void send_rep_status(int status, uint64_t aux);
  void send_field_value(const PacketId &packet_id,
                        uint64_t id, const char *bytes, int nbytes);
  void send_backtrace(const PacketId &packet_id,
                      const std::vector<uint32_t> &stack);
  void send_packet_in(const PacketId &packet_id, int port);
  void send_packet_out(const PacketId &packet_id, int port);
  void send_keep_alive();

  bool in_filters(const PacketId &packet_id) const;

  void packet_ids_count_in(uint64_t packet_id);
  void packet_ids_count_out(uint64_t packet_id);
  uint64_t packet_ids_count_get(uint64_t packet_id) const;

  struct PacketIdKeyHash {
    std::size_t operator()(const PacketId& p) const {
      std::size_t seed = 0;
      boost::hash_combine(seed, p.packet_id);
      boost::hash_combine(seed, p.copy_id);

      return seed;
    }
  };

  class PacketRegisters {
   public:
    void update(uint64_t id, ByteContainer v);

    void query(uint64_t id, const ByteContainer **v) const;

   private:
    std::unordered_map<uint64_t, ByteContainer> registers{};
  };

  nn::socket s;
  std::thread request_thread{};
  bool stop_request_thread{false};
  mutable std::condition_variable cvar_stop_packet_in{};
  mutable std::condition_variable cvar_continue{};
  mutable std::mutex mutex{};
  bool stop_packet_in{false};
  bool can_continue{true};
  bool attached{false};

  bool break_packet_in{false};
  bool break_next{false};
  std::unordered_set<uint64_t> watchpoints{};
  std::unordered_set<PacketId, PacketIdKeyHash> filter{};
  std::unordered_set<uint64_t> filter_all{};

  // find a way to merge it with map below?
  std::unordered_map<uint64_t, uint64_t> packet_ids_count;

  std::unordered_map<PacketId, PacketRegisters, PacketIdKeyHash>
      packet_registers_map{};
  std::unordered_map<PacketId, std::vector<uint32_t>, PacketIdKeyHash>
      packet_ctr_stacks{};

  typedef std::aligned_storage<256>::type ReplyStorage;
  std::unique_ptr<ReplyStorage> rep_storage{new ReplyStorage()};
  size_t rep_size{};

  bool reply_ready{false};
  mutable std::condition_variable cvar_reply_ready{};

  int switch_id{0};
  uint64_t req_id{0};

  std::string addr{};
};

// IMPLEMENTATION

enum MsgType {
  PACKET_IN = 0, PACKET_OUT,
  FIELD_VALUE,
  CONTINUE,
  NEXT,
  GET_VALUE,
  GET_BACKTRACE, BACKTRACE,
  BREAK_PACKET_IN, REMOVE_PACKET_IN,
  STOP_PACKET_IN, RESUME_PACKET_IN,
  FILTER_NOTIFICATIONS,
  SET_WATCHPOINT, UNSET_WATCHPOINT,
  STATUS,
  RESET,
  KEEP_ALIVE,
  ATTACH, DETACH,
};

typedef struct __attribute__((packed)) {
  int type;
  int switch_id;
  uint64_t req_id;
} req_msg_hdr_t;

typedef struct __attribute__((packed)) {
  int type;
  int switch_id;
  uint64_t req_id;
  int status;
  uint64_t aux;
} rep_status_msg_hdr_t;

typedef struct __attribute__((packed)) {
  int type;
  int switch_id;
  uint64_t req_id;
  uint64_t packet_id;
  uint64_t copy_id;
} rep_event_msg_hdr_t;

DebuggerNN::DebuggerNN(const std::string &addr) : s(AF_SP, NN_REP), addr(addr) {
  request_thread = std::thread(&DebuggerNN::request_in, this);
}

DebuggerNN::~DebuggerNN() {
  {
    UniqueLock lock(mutex);
    stop_request_thread = true;
  }
  request_thread.join();
}

void
DebuggerNN::open() {
  s.bind(addr.c_str());
  int to = 100;
  s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof (to));
}

void
DebuggerNN::PacketRegisters::update(uint64_t id, ByteContainer v) {
  registers[id] = std::move(v);
}

void
DebuggerNN::PacketRegisters::query(uint64_t id, const ByteContainer **v) const {
  auto it = registers.find(id);
  if (it == registers.end()) {
    *v = nullptr;
    return;
  }
  *v = &(it->second);
}

void
DebuggerNN::notify_update_generic(const PacketId &packet_id,
                                  uint64_t id, const char *bytes, int nbits) {
  auto it = packet_registers_map.find(packet_id);
  assert(it != packet_registers_map.end());
  PacketRegisters &registers = it->second;
  int nbytes = (nbits + 7) / 8;
  registers.update(id, ByteContainer(bytes, nbytes));
  if (in_filters(packet_id) &&
     (break_next || (watchpoints.find(id) != watchpoints.end()))) {
    can_continue = false;
    break_next = false;
    send_field_value(packet_id, id, bytes, nbytes);
    // notify user
  }
}

namespace {

void
uint32_to_bytes(uint32_t v, char *bytes) {
  for (size_t i = 0; i < sizeof(v); i++) {
    bytes[sizeof(v) - 1 - i] = static_cast<char>(v & 0xFF);
    v >>= 8;
  }
}

}  // end namespace

void
DebuggerNN::notify_update_(const PacketId &packet_id,
                           uint64_t id, const char *bytes, int nbits) {
  UniqueLock lock(mutex);
  wait_for_continue(lock);

  notify_update_generic(packet_id, id, bytes, nbits);
}

void
DebuggerNN::notify_update_(const PacketId &packet_id, uint64_t id, uint32_t v) {
  char bytes[sizeof(v)] = {'\0'};
  uint32_to_bytes(v, bytes);

  notify_update_(packet_id, id, bytes, sizeof(bytes) * 8);
}

void
DebuggerNN::notify_ctr_(const PacketId &packet_id, uint32_t ctr) {
  char bytes[sizeof(ctr)] = {'\0'};
  uint32_to_bytes(ctr, bytes);

  UniqueLock lock(mutex);
  wait_for_continue(lock);

  auto it = packet_ctr_stacks.find(packet_id);
  assert(it != packet_ctr_stacks.end());
  auto &ctr_stack = it->second;
  if (bytes[0] & 0x80)
    ctr_stack.pop_back();
  else
    ctr_stack.push_back(ctr);

#define FIELD_CTR std::numeric_limits<uint64_t>::max()
  notify_update_generic(packet_id, FIELD_CTR, bytes, sizeof(bytes) * 8);
#undef FIELD_CTR
}

void
DebuggerNN::packet_in_(const PacketId &packet_id, int port) {
  UniqueLock lock(mutex);

  while (!can_continue ||
        (stop_packet_in && (packet_ids_count_get(packet_id.packet_id) == 0))) {
    wait_for_continue(lock);
    if (packet_ids_count_get(packet_id.packet_id) == 0)
      wait_for_resume_packet_in(lock);
  }

  auto it = packet_registers_map.find(packet_id);
  assert(it == packet_registers_map.end());
  packet_registers_map.emplace_hint(packet_registers_map.end(),
                                    packet_id, PacketRegisters());

  auto it2 = packet_ctr_stacks.find(packet_id);
  assert(it2 == packet_ctr_stacks.end());
  packet_ctr_stacks.emplace_hint(packet_ctr_stacks.end(),
                                 packet_id, std::vector<uint32_t>());

  packet_ids_count_in(packet_id.packet_id);

  if (in_filters(packet_id) && (break_next || break_packet_in)) {
    can_continue = false;
    break_next = false;
    send_packet_in(packet_id, port);
  }
}

void
DebuggerNN::packet_out_(const PacketId &packet_id, int port) {
  UniqueLock lock(mutex);

  wait_for_continue(lock);

  auto it = packet_registers_map.find(packet_id);
  assert(it != packet_registers_map.end());
  packet_registers_map.erase(it);

  auto it2 = packet_ctr_stacks.find(packet_id);
  assert(it2 != packet_ctr_stacks.end());
  packet_ctr_stacks.erase(it2);

  packet_ids_count_out(packet_id.packet_id);

  (void) port;
}

void
DebuggerNN::config_change_() {
  BMLOG_DEBUG("Dbg had been notified of config change");
  UniqueLock lock(mutex);
  if (!attached) return;
  // We treat a config change as any other request that needs to be sent to the
  // client. While this call does not return the debugger still has access to
  // all the old register values to we are fine if the client asks to see the
  // value of a field.
  wait_for_continue(lock);
  reset_state();
  // TODO(antonin): standardize those codes
  send_rep_status(999, 0);
}

void
DebuggerNN::packet_ids_count_in(uint64_t packet_id) {
  const auto p = packet_ids_count.insert(std::make_pair(packet_id, 1));
  if (!p.second) {
    p.first->second += 1;
  }
}

void
DebuggerNN::packet_ids_count_out(uint64_t packet_id) {
  auto it = packet_ids_count.find(packet_id);
  assert(it != packet_ids_count.end());
  assert(it->second > 0);
  it->second--;
  if (it->second == 0)
    packet_ids_count.erase(it);
}

uint64_t
DebuggerNN::packet_ids_count_get(uint64_t packet_id) const {
  auto it = packet_ids_count.find(packet_id);
  if (it == packet_ids_count.end())
    return 0;
  return it->second;
}

bool
DebuggerNN::in_filters(const PacketId &packet_id) const {
  if (filter.empty() && filter_all.empty())
    return true;
  if (filter_all.find(packet_id.packet_id) != filter_all.end())
    return true;
  if (filter.find(packet_id) != filter.end())
    return true;
  return false;
}

void
DebuggerNN::wait_for_continue(UniqueLock &lock) const {
  while (!can_continue) {
    cvar_continue.wait(lock);
  }
}

void
DebuggerNN::wait_for_resume_packet_in(UniqueLock &lock) const {
  while (stop_packet_in) {
    cvar_stop_packet_in.wait(lock);
  }
}

void
DebuggerNN::req_continue(const char *data) {
  (void) data;
  BMLOG_TRACE("Dbg cmd received: continue");
  UniqueLock lock(mutex);
  can_continue = true;
  cvar_continue.notify_all();
}

void
DebuggerNN::req_next(const char *data) {
  (void) data;
  BMLOG_TRACE("Dbg cmd received: next");
  UniqueLock lock(mutex);
  can_continue = true;
  break_next = true;
  cvar_continue.notify_all();
}

void
DebuggerNN::req_get_value(const char *data) {
  typedef struct : req_msg_hdr_t {
    uint64_t packet_id;
    uint64_t copy_id;
    uint64_t id;
  } __attribute__((packed)) msg_hdr_t;
  const msg_hdr_t *msg_hdr = reinterpret_cast<const msg_hdr_t *>(data);
  BMLOG_TRACE("Dbg cmd received: get value");
  UniqueLock lock(mutex);
  auto it = packet_registers_map.find({msg_hdr->packet_id, msg_hdr->copy_id});
  if (it == packet_registers_map.end()) {
    // no such packet in the system
    send_rep_status(1, 0);
    return;
  }
  PacketRegisters &registers = it->second;
  const ByteContainer *bc;
  registers.query(msg_hdr->id, &bc);
  if (bc == nullptr) {
    send_rep_status(2, 0);
    return;
  }
  send_field_value({msg_hdr->packet_id, msg_hdr->copy_id},
                   msg_hdr->id, bc->data(), bc->size());
}

void
DebuggerNN::req_get_backtrace(const char *data) {
  typedef struct : req_msg_hdr_t {
    uint64_t packet_id;
    uint64_t copy_id;
  } __attribute__((packed)) msg_hdr_t;
  const msg_hdr_t *msg_hdr = reinterpret_cast<const msg_hdr_t *>(data);
  BMLOG_TRACE("Dbg cmd received: get backtrace");
  UniqueLock lock(mutex);
  auto it = packet_ctr_stacks.find({msg_hdr->packet_id, msg_hdr->copy_id});
  if (it == packet_ctr_stacks.end()) {
    // no such packet in the system
    send_rep_status(1, 0);
    return;
  }
  const auto &stack = it->second;
  send_backtrace({msg_hdr->packet_id, msg_hdr->copy_id}, stack);
}

void
DebuggerNN::req_break_packet_in(const char *data) {
  (void) data;
  BMLOG_TRACE("Dbg cmd received: break packet in");
  UniqueLock lock(mutex);
  break_packet_in = true;
  send_rep_status(0, 0);
}

void
DebuggerNN::req_remove_packet_in(const char *data) {
  (void) data;
  BMLOG_TRACE("Dbg cmd received: remove packet in");
  UniqueLock lock(mutex);
  if (!break_packet_in) {
    send_rep_status(1, 0);
  } else {
    break_packet_in = false;
    send_rep_status(0, 0);
  }
}

void
DebuggerNN::req_stop_packet_in(const char *data) {
  (void) data;
  BMLOG_TRACE("Dbg cmd received: stop packet in");
  UniqueLock lock(mutex);
  stop_packet_in = true;
  send_rep_status(0, 0);
}

void
DebuggerNN::req_resume_packet_in(const char *data) {
  (void) data;
  BMLOG_TRACE("Dbg cmd received: resume packet in");
  UniqueLock lock(mutex);
  if (!stop_packet_in) {
    send_rep_status(1, 0);
  } else {
    stop_packet_in = false;
    cvar_stop_packet_in.notify_all();
    send_rep_status(0, 0);
  }
}

void
DebuggerNN::req_filter_notifications(const char *data) {
  typedef struct : req_msg_hdr_t {
    int nb;
  } __attribute__((packed)) msg_hdr_t;
  const msg_hdr_t *msg_hdr = reinterpret_cast<const msg_hdr_t *>(data);
  int nb = msg_hdr->nb;
  typedef struct {
    uint64_t packet_id;
    uint64_t copy_id;
  } __attribute__((packed)) one_id_t;
  BMLOG_TRACE("Dbg cmd received: filter notifications");
  const char *data_ = data + sizeof(*msg_hdr);;
  UniqueLock lock(mutex);
  filter.clear();
  filter_all.clear();
  for (int i = 0; i < nb; i++) {
    const one_id_t *one_id = reinterpret_cast<const one_id_t *>(data_);
    if (one_id->copy_id == std::numeric_limits<uint64_t>::max())
    // if (one_id->copy_id == static_cast<uint64_t>(-1))
      filter_all.insert(one_id->packet_id);
    else
      filter.insert({one_id->packet_id, one_id->copy_id});
    data_ += sizeof(*one_id);
  }
  send_rep_status(0, 0);
}

void
DebuggerNN::req_set_watchpoint(const char *data) {
  typedef struct : req_msg_hdr_t {
    uint64_t id;
  } __attribute__((packed)) msg_hdr_t;
  const msg_hdr_t *msg_hdr = reinterpret_cast<const msg_hdr_t *>(data);
  BMLOG_TRACE("Dbg cmd received: set watchpoint");
  UniqueLock lock(mutex);
  watchpoints.insert(msg_hdr->id);
  send_rep_status(0, 0);
}

void
DebuggerNN::req_unset_watchpoint(const char *data) {
  typedef struct : req_msg_hdr_t {
    uint64_t id;
  } __attribute__((packed)) msg_hdr_t;
  const msg_hdr_t *msg_hdr = reinterpret_cast<const msg_hdr_t *>(data);
  BMLOG_TRACE("Dbg cmd received: unset watchpoint");
  UniqueLock lock(mutex);
  size_t res = watchpoints.erase(msg_hdr->id);
  if (res > 0)
    send_rep_status(0, 0);
  else
    send_rep_status(1, 0);
}

void
DebuggerNN::reset_state() {
  watchpoints.clear();
  filter.clear();
  filter_all.clear();
  break_packet_in = false;
  req_id = 0;  // needed ?
  stop_packet_in = false;
  cvar_stop_packet_in.notify_all();
}

void
DebuggerNN::req_reset(const char *data) {
  (void) data;
  UniqueLock lock(mutex);
  reset_state();
  send_rep_status(0, 0);
}

void
DebuggerNN::req_attach(const char *data) {
  (void) data;
  BMLOG_DEBUG("Dbg requires attach");
  UniqueLock lock(mutex);
  if (attached) {  // already attached
    BMLOG_DEBUG("Dbg already attached");
    send_rep_status(1, 0);
    return;
  }
  attached = true;
  can_continue = false;
  reset_state();
  send_rep_status(0, 0);
}

void
DebuggerNN::req_detach(const char *data) {
  (void) data;
  BMLOG_DEBUG("Dbg requires detach");
  UniqueLock lock(mutex);
  if (!attached) {  // already detached
    BMLOG_DEBUG("Dbg already detached");
    send_rep_status(1, 0);
    return;
  }
  attached = false;
  reset_state();
  can_continue = true;
  cvar_continue.notify_all();
  send_rep_status(0, 0);
}

void
DebuggerNN::send_rep_status(int status, uint64_t aux) {
  rep_status_msg_hdr_t *msg_hdr = reinterpret_cast<rep_status_msg_hdr_t *>(
      rep_storage.get());;
  msg_hdr->type = STATUS;
  msg_hdr->switch_id = switch_id;
  msg_hdr->req_id = req_id;
  msg_hdr->status = status;
  msg_hdr->aux = aux;
  assert(!reply_ready);
  rep_size = sizeof(*msg_hdr);
  reply_ready = true;
}

void
DebuggerNN::send_field_value(const PacketId &packet_id,
                           uint64_t id, const char *bytes, int nbytes) {
  typedef struct : rep_event_msg_hdr_t {
    uint64_t id;
    int nbytes;
  } __attribute__((packed)) msg_hdr_t;
  msg_hdr_t *msg_hdr = reinterpret_cast<msg_hdr_t *>(rep_storage.get());
  char *buf = reinterpret_cast<char *>(rep_storage.get());
  msg_hdr->type = FIELD_VALUE;
  msg_hdr->switch_id = switch_id;
  msg_hdr->req_id = req_id;
  msg_hdr->packet_id = packet_id.packet_id;
  msg_hdr->copy_id = packet_id.copy_id;
  msg_hdr->id = id;
  msg_hdr->nbytes = nbytes;
  memcpy(buf + sizeof(*msg_hdr), bytes, nbytes);
  assert(!reply_ready);
  rep_size = sizeof(*msg_hdr) + nbytes;
  reply_ready = true;
  cvar_reply_ready.notify_one();
}

void
DebuggerNN::send_backtrace(const PacketId &packet_id,
                           const std::vector<uint32_t> &stack) {
  typedef struct : rep_event_msg_hdr_t {
    int nb;
  } __attribute__((packed)) msg_hdr_t;
  msg_hdr_t *msg_hdr = reinterpret_cast<msg_hdr_t *>(rep_storage.get());
  char *buf = reinterpret_cast<char *>(rep_storage.get());
  msg_hdr->type = BACKTRACE;
  msg_hdr->switch_id = switch_id;
  msg_hdr->req_id = req_id;
  msg_hdr->packet_id = packet_id.packet_id;
  msg_hdr->copy_id = packet_id.copy_id;
  msg_hdr->nb = static_cast<int>(stack.size());
  size_t ctrs_size = sizeof(uint32_t) * stack.size();
  memcpy(buf + sizeof(*msg_hdr), stack.data(), ctrs_size);
  assert(!reply_ready);
  rep_size = sizeof(*msg_hdr) + ctrs_size;
  reply_ready = true;
  cvar_reply_ready.notify_one();
}

void
DebuggerNN::send_packet_in(const PacketId &packet_id, int port) {
  typedef struct : rep_event_msg_hdr_t {
    int port;
  } __attribute__((packed)) msg_hdr_t;
  msg_hdr_t *msg_hdr = reinterpret_cast<msg_hdr_t *>(rep_storage.get());
  msg_hdr->type = PACKET_IN;
  msg_hdr->switch_id = switch_id;
  msg_hdr->req_id = req_id;
  msg_hdr->packet_id = packet_id.packet_id;
  msg_hdr->copy_id = packet_id.copy_id;
  msg_hdr->port = port;
  assert(!reply_ready);
  rep_size = sizeof(*msg_hdr);
  reply_ready = true;
  cvar_reply_ready.notify_one();
}

void
DebuggerNN::send_packet_out(const PacketId &packet_id, int port) {
  typedef struct : rep_event_msg_hdr_t {
    int port;
  } __attribute__((packed)) msg_hdr_t;
  msg_hdr_t *msg_hdr = reinterpret_cast<msg_hdr_t *>(rep_storage.get());
  msg_hdr->type = PACKET_OUT;
  msg_hdr->switch_id = switch_id;
  msg_hdr->req_id = req_id;
  msg_hdr->packet_id = packet_id.packet_id;
  msg_hdr->copy_id = packet_id.copy_id;
  msg_hdr->port = port;
  assert(!reply_ready);
  rep_size = sizeof(*msg_hdr);
  reply_ready = true;
  cvar_reply_ready.notify_one();
}

void
DebuggerNN::send_keep_alive() {
  // reusing status for convenience
  rep_status_msg_hdr_t *msg_hdr = reinterpret_cast<rep_status_msg_hdr_t *>(
      rep_storage.get());
  msg_hdr->type = KEEP_ALIVE;
  msg_hdr->switch_id = switch_id;
  msg_hdr->req_id = req_id;
  // not used
  msg_hdr->status = 0;
  msg_hdr->aux = 0;
  assert(!reply_ready);
  rep_size = sizeof(*msg_hdr);
  reply_ready = true;
  cvar_reply_ready.notify_one();
}

void
DebuggerNN::request_dispatch(const int msg_type, const char *buf) {
  switch (msg_type) {
  case CONTINUE:
    req_continue(buf);
    break;
  case NEXT:
    req_next(buf);
    break;
  case GET_VALUE:
    req_get_value(buf);
    break;
  case GET_BACKTRACE:
    req_get_backtrace(buf);
    break;
  case BREAK_PACKET_IN:
    req_break_packet_in(buf);
    break;
  case REMOVE_PACKET_IN:
    req_remove_packet_in(buf);
    break;
  case STOP_PACKET_IN:
    req_stop_packet_in(buf);
    break;
  case RESUME_PACKET_IN:
    req_resume_packet_in(buf);
    break;
  case FILTER_NOTIFICATIONS:
    req_filter_notifications(buf);
    break;
  case SET_WATCHPOINT:
    req_set_watchpoint(buf);
    break;
  case UNSET_WATCHPOINT:
    req_unset_watchpoint(buf);
    break;
  case RESET:
    req_reset(buf);
    break;
  case ATTACH:
    req_attach(buf);
    break;
  case DETACH:
    req_detach(buf);
    break;
  default:
    assert(0);
  }
}

void
DebuggerNN::request_in() {
  while (true) {
    {
      UniqueLock lock(mutex);
      if (stop_request_thread)
        return;
    }

    // see http://stackoverflow.com/questions/29287990/invalid-cast-from-char-to-int
    // too big?
    std::aligned_storage<512>::type storage;
    req_msg_hdr_t *msg_hdr = reinterpret_cast<req_msg_hdr_t *>(&storage);
    char *buf = reinterpret_cast<char *>(&storage);

    int bytes = s.recv(buf, sizeof(storage), 0);
    if (bytes <= 0){
      continue;
    }

    {
      UniqueLock lock(mutex);
      reply_ready = false;
    }

    req_id = msg_hdr->req_id;

    request_dispatch(msg_hdr->type, buf);

    {
      UniqueLock lock(mutex);
      using std::chrono::system_clock;
      using std::chrono::duration_cast;
      using std::chrono::milliseconds;
      system_clock::time_point start = system_clock::now();
      while (!reply_ready) {
        cvar_reply_ready.wait_for(lock, milliseconds(100));
        system_clock::time_point now = system_clock::now();
        if (!reply_ready &&
            duration_cast<milliseconds>(now - start).count() > 90) {
          can_continue = false;
          break_next = false;
          send_keep_alive();
          assert(reply_ready);
        }
      }

      const char *msg = reinterpret_cast<const char *>(rep_storage.get());
      s.send(msg, rep_size, 0);
    }
  }
}

// TODO(antonin): check if allowed by Google style guidelines
DebuggerIface *Debugger::debugger = new DebuggerDummy();

bool Debugger::is_init = false;

void
Debugger::init_debugger(const std::string &addr) {
  if (is_init) return;
  is_init = true;
  DebuggerDummy *dummy = dynamic_cast<DebuggerDummy *>(debugger);
  assert(dummy);
  delete dummy;
  DebuggerNN *debugger_nn = new DebuggerNN(addr);
  debugger_nn->open();
  debugger = debugger_nn;
}

std::string
Debugger::get_addr() {
  if (!is_init) return "";
  return debugger->get_addr();
}

}  // namespace bm
