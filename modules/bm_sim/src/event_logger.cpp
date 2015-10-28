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

#include <cstring>

#include <bm_sim/event_logger.h>

#include "bm_sim/parser.h"
#include "bm_sim/deparser.h"
#include "bm_sim/tables.h"
#include "bm_sim/conditionals.h"
#include "bm_sim/actions.h"

enum EventType {
  PACKET_IN = 0, PACKET_OUT,
  PARSER_START, PARSER_DONE, PARSER_EXTRACT,
  DEPARSER_START, DEPARSER_DONE, DEPARSER_EMIT,
  CHECKSUM_UPDATE,
  PIPELINE_START, PIPELINE_DONE,
  CONDITION_EVAL, TABLE_HIT, TABLE_MISS,
  ACTION_EXECUTE
};


typedef struct __attribute__((packed)) {
  int type;
  int switch_id; // TODO
  unsigned long long sig;
  unsigned long long id;
  unsigned long long copy_id;
} msg_hdr_t;

static inline void fill_msg_hdr(EventType type, const Packet &packet,
				msg_hdr_t *msg_hdr) {
  msg_hdr->type = (int) type;
  msg_hdr->switch_id = 0;
  msg_hdr->sig = packet.get_signature();
  msg_hdr->id = packet.get_packet_id();
  msg_hdr->copy_id = packet.get_copy_id();
}

void EventLogger::packet_in(const Packet &packet) {
  typedef struct : msg_hdr_t {
    int port_in;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PACKET_IN, packet, &msg);
  msg.port_in = packet.get_ingress_port();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::packet_out(const Packet &packet) {
  typedef struct : msg_hdr_t {
    int port_out;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PACKET_OUT, packet, &msg);
  msg.port_out = packet.get_egress_port();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::parser_start(const Packet &packet, 
			       const Parser &parser) {
  typedef struct : msg_hdr_t {
    int parser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_START, packet, &msg);
  msg.parser_id = parser.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::parser_done(const Packet &packet,
			      const Parser &parser) {
  typedef struct : msg_hdr_t {
    int parser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_DONE, packet, &msg);
  msg.parser_id = parser.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::parser_extract(const Packet &packet,
				 header_id_t header) {
  typedef struct : msg_hdr_t {
    int header_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_EXTRACT, packet, &msg);
  msg.header_id = header;
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::deparser_start(const Packet &packet,
				 const Deparser &deparser) {
  typedef struct : msg_hdr_t {
    int deparser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_START, packet, &msg);
  msg.deparser_id = deparser.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::deparser_done(const Packet &packet,
				const Deparser &deparser) {
  typedef struct : msg_hdr_t {
    int deparser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_DONE, packet, &msg);
  msg.deparser_id = deparser.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::deparser_emit(const Packet &packet,
				header_id_t header) {
  typedef struct : msg_hdr_t {
    int header_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_EMIT, packet, &msg);
  msg.header_id = header;
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::checksum_update(const Packet &packet,
				  const Checksum &checksum) {
  typedef struct : msg_hdr_t {
    int checksum_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::CHECKSUM_UPDATE, packet, &msg);
  msg.checksum_id = checksum.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::pipeline_start(const Packet &packet,
				 const Pipeline &pipeline) {
  typedef struct : msg_hdr_t {
    int pipeline_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PIPELINE_START, packet, &msg);
  msg.pipeline_id = pipeline.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::pipeline_done(const Packet &packet,
				const Pipeline &pipeline) {
  typedef struct : msg_hdr_t {
    int pipeline_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PIPELINE_DONE, packet, &msg);
  msg.pipeline_id = pipeline.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::condition_eval(const Packet &packet,
				 const Conditional &cond, bool result) {
  typedef struct : msg_hdr_t {
    int condition_id;
    int result;  // 0 (true) or 1 (false);
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::CONDITION_EVAL, packet, &msg);
  msg.condition_id = cond.get_id();
  msg.result = result;
  transport_instance->send((char *) &msg, sizeof(msg));
};

// static inline size_t get_pascal_str_size(const ByteContainer &src) {
//   return 1 + src.size();
// }

// static inline void dump_pascal_str(const ByteContainer &src, char *dst) {
//   assert(src.size() < 256);
//   dst[0] = (char) src.size();
//   std::copy(src.begin(), src.end(), dst);
// }

void EventLogger::table_hit(const Packet &packet,
			    const MatchTableAbstract &table,
			    entry_handle_t handle) {
  typedef struct : msg_hdr_t {
    int table_id;
    int entry_hdl;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::TABLE_HIT, packet, &msg);
  msg.table_id = table.get_id();
  msg.entry_hdl = (int) handle;
  transport_instance->send((char *) &msg, sizeof(msg));
};

void EventLogger::table_miss(const Packet &packet,
			     const MatchTableAbstract &table) {
  typedef struct : msg_hdr_t {
    int table_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::TABLE_MISS, packet, &msg);
  msg.table_id = table.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
}

void EventLogger::action_execute(const Packet &packet,
				 const ActionFn &action_fn,
				 const ActionData &action_data) {
  typedef struct : msg_hdr_t {
    int action_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::ACTION_EXECUTE, packet, &msg);
  msg.action_id = action_fn.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
  // to costly to send action data?
  (void) action_data;
};

// TODO: move this?
EventLogger *event_logger = new EventLogger(std::move(TransportIface::create_instance<TransportNULL>("")));
