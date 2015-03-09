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
void EventLogger<Transport>::log_parser_start() { };

template <typename Transport>
void EventLogger<Transport>::log_parser_extract() { };

template <typename Transport>
void EventLogger<Transport>::log_deparser_start() { };

template <typename Transport>
void EventLogger<Transport>::log_deparser_deparse() { };

template <typename Transport>
void EventLogger<Transport>::log_cond_eval() { };

template <typename Transport>
void EventLogger<Transport>::log_table_hit() {
  transport_instance->send("table hit!");
};

template <typename Transport>
void EventLogger<Transport>::log_table_miss() { };

template <typename Transport>
void EventLogger<Transport>::log_action_execute() { };


template class EventLogger<TransportNanomsg>;
template class EventLogger<TransportNULL>;
template class EventLogger<TransportSTDOUT>;
