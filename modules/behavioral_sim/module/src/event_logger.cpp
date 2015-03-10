#include <iostream>

#include <cstring>

#include <nanomsg/pubsub.h>

#include <behavioral_sim/event_logger.h>

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

template <typename Transport>
enum EventLogger<Transport>::EventType {
  PACKET_IN = 0,
  TABLE_HIT
};


// TODO: wanted it in class scope, possible?
typedef struct __attribute__((packed)) {
  int type;
  int switch_id; // TODO
  unsigned long long sig;
  unsigned long long id;
  unsigned long long copy_id;
} msg_hdr_t;

template <typename Transport>
void EventLogger<Transport>::packet_in(const Packet &packet) {
  typedef struct : msg_hdr_t {
    
  } __attribute__((packed)) msg_t;

  msg_t msg;
  msg.type = (int) PACKET_IN;
  msg.switch_id = 0;
  msg.sig = packet.get_signature();
  msg.id = packet.get_packet_id();
  msg.copy_id = packet.get_copy_id();

  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::parser_start(const Packet &packet) { };

template <typename Transport>
void EventLogger<Transport>::parser_extract(const Packet &packet) { };

template <typename Transport>
void EventLogger<Transport>::deparser_start(const Packet &packet) { };

template <typename Transport>
void EventLogger<Transport>::deparser_deparse(const Packet &packet) { };

template <typename Transport>
void EventLogger<Transport>::cond_eval(const Packet &packet) { };

template <typename Transport>
void EventLogger<Transport>::table_hit(const Packet &packet,
				       const std::string &table_name) {
  typedef struct : msg_hdr_t {
    int table_id; // TODO
  } __attribute__((packed)) msg_t;

  msg_t msg;
  msg.type = (int) TABLE_HIT;
  msg.switch_id = 0;
  msg.sig = packet.get_signature();
  msg.id = packet.get_packet_id();
  msg.copy_id = packet.get_copy_id();
  msg.table_id = 0;

  transport_instance->send((char *) &msg, sizeof(msg));
};

template <typename Transport>
void EventLogger<Transport>::table_miss(const Packet &packet) { };

template <typename Transport>
void EventLogger<Transport>::action_execute(const Packet &packet) { };


template class EventLogger<TransportNanomsg>;
template class EventLogger<TransportNULL>;
template class EventLogger<TransportSTDOUT>;
