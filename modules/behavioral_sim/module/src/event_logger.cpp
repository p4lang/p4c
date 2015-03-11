#include <iostream>

#include <cstring>

#include <nanomsg/pubsub.h>

#include <behavioral_sim/event_logger.h>

#include "behavioral_sim/parser.h"
#include "behavioral_sim/deparser.h"
#include "behavioral_sim/tables.h"
#include "behavioral_sim/conditionals.h"
#include "behavioral_sim/actions.h"

int TransportSTDOUT::open(const std::string &name) {
  return 0;
}

int TransportSTDOUT::send(const std::string &msg) const {
  std::cout << msg << std::endl;
  return 0;
};

int TransportSTDOUT::send(const char *msg, int len) const {
  std::cout << msg << std::endl;
  return 0;
};

TransportNanomsg::TransportNanomsg()
  : s(AF_SP, NN_PUB) {}

int TransportNanomsg::open(const std::string &name) {
  // TODO: catch exception
  s.bind(name.c_str());
  return 0;
}

int TransportNanomsg::send(const std::string &msg) const {
  return 0;
};

int TransportNanomsg::send(const char *msg, int len) const {
  s.send(msg, len, 0);
  return 0;
};

enum EventType {
  PACKET_IN = 0, PACKET_OUT,
  PARSER_START, PARSER_DONE, PARSER_EXTRACT,
  DEPARSER_START, DEPARSER_DONE, DEPARSER_EMIT,
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

template <typename Transport>
void EventLogger<Transport>::packet_in(const Packet &packet) {
  typedef struct : msg_hdr_t {
    int port_in;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PACKET_IN, packet, &msg);
  msg.port_in = packet.get_ingress_port();
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::packet_out(const Packet &packet) {
  typedef struct : msg_hdr_t {
    int port_out;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PACKET_OUT, packet, &msg);
  msg.port_out = packet.get_egress_port();
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::parser_start(const Packet &packet, 
					  const Parser &parser) {
  typedef struct : msg_hdr_t {
    int parser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_START, packet, &msg);
  msg.parser_id = parser.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::parser_done(const Packet &packet,
					 const Parser &parser) {
  typedef struct : msg_hdr_t {
    int parser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_DONE, packet, &msg);
  msg.parser_id = parser.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::parser_extract(const Packet &packet,
					    header_id_t header) {
  typedef struct : msg_hdr_t {
    int header_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PARSER_EXTRACT, packet, &msg);
  msg.header_id = header;
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::deparser_start(const Packet &packet,
					    const Deparser &deparser) {
  typedef struct : msg_hdr_t {
    int deparser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_START, packet, &msg);
  msg.deparser_id = deparser.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::deparser_done(const Packet &packet,
					   const Deparser &deparser) {
  typedef struct : msg_hdr_t {
    int deparser_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_DONE, packet, &msg);
  msg.deparser_id = deparser.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::deparser_emit(const Packet &packet,
					   header_id_t header) {
  typedef struct : msg_hdr_t {
    int header_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::DEPARSER_EMIT, packet, &msg);
  msg.header_id = header;
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::pipeline_start(const Packet &packet,
					    const Pipeline &pipeline) {
  typedef struct : msg_hdr_t {
    int pipeline_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PIPELINE_START, packet, &msg);
  msg.pipeline_id = pipeline.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::pipeline_done(const Packet &packet,
					   const Pipeline &pipeline) {
  typedef struct : msg_hdr_t {
    int pipeline_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::PIPELINE_DONE, packet, &msg);
  msg.pipeline_id = pipeline.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::condition_eval(const Packet &packet,
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

template <typename Transport>
void EventLogger<Transport>::table_hit(const Packet &packet,
				       const MatchTable &table,
				       const MatchEntry &entry) {
  typedef struct : msg_hdr_t {
    int table_id;
    int entry_hdl;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::TABLE_HIT, packet, &msg);
  msg.table_id = table.get_id();
  msg.entry_hdl = (int) table.get_entry_handle(entry);
  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::table_miss(const Packet &packet,
					const MatchTable &table) {
  typedef struct : msg_hdr_t {
    int table_id;
  } __attribute__((packed)) msg_t;

  msg_t msg;
  fill_msg_hdr(EventType::TABLE_MISS, packet, &msg);
  msg.table_id = table.get_id();
  transport_instance->send((char *) &msg, sizeof(msg));
}

template <typename Transport>
void EventLogger<Transport>::action_execute(const Packet &packet) { };


template class EventLogger<TransportNanomsg>;
template class EventLogger<TransportNULL>;
template class EventLogger<TransportSTDOUT>;
