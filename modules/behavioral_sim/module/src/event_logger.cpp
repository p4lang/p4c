#include <iostream>

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

int TransportNanomsg::open(const std::string &name) {
  return 0;
}

int TransportNanomsg::send(const std::string &msg) const {
  return 0;
};

int TransportNanomsg::send(const char *msg, int len) const {
  return 0;
};

template <typename Transport>
void EventLogger<Transport>::packet_in(const Packet &packet) {
  
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
void EventLogger<Transport>::table_hit(const Packet &packet) {
  transport_instance->send("table hit!");
};

template <typename Transport>
void EventLogger<Transport>::table_miss(const Packet &packet) { };

template <typename Transport>
void EventLogger<Transport>::action_execute(const Packet &packet) { };


template class EventLogger<TransportNanomsg>;
template class EventLogger<TransportNULL>;
template class EventLogger<TransportSTDOUT>;
