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

#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/deparser.h>
#include <bm/bm_sim/tables.h>
#include <bm/bm_sim/conditionals.h>
#include <bm/bm_sim/actions.h>

#include <cstring>

namespace bm {

enum EventType {
  PACKET_IN = 0, PACKET_OUT,
  PARSER_START, PARSER_DONE, PARSER_EXTRACT,
  DEPARSER_START, DEPARSER_DONE, DEPARSER_EMIT,
  CHECKSUM_UPDATE,
  PIPELINE_START, PIPELINE_DONE,
  CONDITION_EVAL, TABLE_HIT, TABLE_MISS,
  ACTION_EXECUTE,
  CONFIG_CHANGE = 999
};

typedef struct __attribute__((packed)) {
  int type;
  int switch_id;
  int cxt_id;
  uint64_t sig;
  uint64_t id;
  uint64_t copy_id;
} msg_hdr_t;

namespace {

void
fill_msg_hdr(EventType type, int device_id,
             const Packet &packet, msg_hdr_t *msg_hdr) {
  msg_hdr->type = static_cast<int>(type);
  msg_hdr->switch_id = device_id;
  msg_hdr->cxt_id = packet.get_context();
  msg_hdr->sig = packet.get_signature();
  msg_hdr->id = packet.get_packet_id();
  msg_hdr->copy_id = packet.get_copy_id();
}

}  // namespace

void
EventLogger::packet_in(const Packet &packet) {
  typedef struct : msg_hdr_t {
    int port_in;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PACKET_IN, device_id, packet, &msg);
  msg.port_in = packet.get_ingress_port();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::packet_out(const Packet &packet) {
  typedef struct : msg_hdr_t {
    int port_out;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PACKET_OUT, device_id, packet, &msg);
  msg.port_out = packet.get_egress_port();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::parser_start(const Packet &packet, const Parser &parser) {
  typedef struct : msg_hdr_t {
    int parser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_START, device_id, packet, &msg);
  msg.parser_id = parser.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::parser_done(const Packet &packet, const Parser &parser) {
  typedef struct : msg_hdr_t {
    int parser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_DONE, device_id, packet, &msg);
  msg.parser_id = parser.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::parser_extract(const Packet &packet, header_id_t header) {
  typedef struct : msg_hdr_t {
    int header_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_EXTRACT, device_id, packet, &msg);
  msg.header_id = header;
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::deparser_start(const Packet &packet, const Deparser &deparser) {
  typedef struct : msg_hdr_t {
    int deparser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_START, device_id, packet, &msg);
  msg.deparser_id = deparser.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::deparser_done(const Packet &packet, const Deparser &deparser) {
  typedef struct : msg_hdr_t {
    int deparser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_DONE, device_id, packet, &msg);
  msg.deparser_id = deparser.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::deparser_emit(const Packet &packet, header_id_t header) {
  typedef struct : msg_hdr_t {
    int header_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_EMIT, device_id, packet, &msg);
  msg.header_id = header;
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::checksum_update(const Packet &packet, const Checksum &checksum) {
  typedef struct : msg_hdr_t {
    int checksum_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::CHECKSUM_UPDATE, device_id, packet, &msg);
  msg.checksum_id = checksum.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::pipeline_start(const Packet &packet, const Pipeline &pipeline) {
  typedef struct : msg_hdr_t {
    int pipeline_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PIPELINE_START, device_id, packet, &msg);
  msg.pipeline_id = pipeline.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::pipeline_done(const Packet &packet, const Pipeline &pipeline) {
  typedef struct : msg_hdr_t {
    int pipeline_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PIPELINE_DONE, device_id, packet, &msg);
  msg.pipeline_id = pipeline.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::condition_eval(const Packet &packet,
                            const Conditional &cond, bool result) {
  typedef struct : msg_hdr_t {
    int condition_id;
    int result;  // 0 (true) or 1 (false);
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::CONDITION_EVAL, device_id, packet, &msg);
  msg.condition_id = cond.get_id();
  msg.result = result;
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

// static inline size_t get_pascal_str_size(const ByteContainer &src) {
//   return 1 + src.size();
// }

// static inline void dump_pascal_str(const ByteContainer &src, char *dst) {
//   assert(src.size() < 256);
//   dst[0] = (char) src.size();
//   std::copy(src.begin(), src.end(), dst);
// }

void
EventLogger::table_hit(const Packet &packet, const MatchTableAbstract &table,
                       entry_handle_t handle) {
  typedef struct : msg_hdr_t {
    int table_id;
    int entry_hdl;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::TABLE_HIT, device_id, packet, &msg);
  msg.table_id = table.get_id();
  msg.entry_hdl = static_cast<int>(handle);
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::table_miss(const Packet &packet, const MatchTableAbstract &table) {
  typedef struct : msg_hdr_t {
    int table_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::TABLE_MISS, device_id, packet, &msg);
  msg.table_id = table.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

void
EventLogger::action_execute(const Packet &packet,
                            const ActionFn &action_fn,
                            const ActionData &action_data) {
  typedef struct : msg_hdr_t {
    int action_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::ACTION_EXECUTE, device_id, packet, &msg);
  msg.action_id = action_fn.get_id();
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
  // to costly to send action data?
  (void) action_data;
}

void
EventLogger::config_change() {
  msg_hdr_t msg;
  std::memset(&msg, 0, sizeof(msg));
  msg.type = static_cast<int>(EventType::CONFIG_CHANGE);
  msg.switch_id = device_id;
  transport_instance->send(reinterpret_cast<char *>(&msg), sizeof(msg));
}

// TODO(antonin): move this?
EventLogger *event_logger =
    new EventLogger(TransportIface::make_dummy());

}  // namespace bm
