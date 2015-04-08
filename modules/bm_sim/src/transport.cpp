#include <iostream>

#include <nanomsg/pubsub.h>

#include "bm_sim/transport.h"

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
