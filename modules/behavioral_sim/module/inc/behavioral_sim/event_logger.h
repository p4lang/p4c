#ifndef _BM_EVENT_LOGGER_H_
#define _BM_EVENT_LOGGER_H_

#include <string>
#include <memory>

#include "behavioral_utils/nn.h"

#include "packet.h"

class TransportIface {
public:
  virtual int open(const std::string &name) = 0;

  virtual int send(const std::string &msg) const = 0;
  virtual int send(const char *msg, int len) const = 0;
};

class TransportNanomsg : public TransportIface {
public:
  TransportNanomsg();

  int open(const std::string &name) override;

  int send(const std::string &msg) const override;
  int send(const char *msg, int len) const override;

private:
  nn::socket s;
};

class TransportSTDOUT : public TransportIface {
public:
  int open(const std::string &name) override;

  int send(const std::string &msg) const override;

  int send(const char *msg, int len) const override;
};

class TransportNULL : public TransportIface {
public:
  int open(const std::string &name) override { return 0; }

  int send(const std::string &msg) const override { return 0; }
  int send(const char *msg, int len) const override { return 0; }
};

template <typename Transport>
class EventLogger {
public:
  EventLogger(const std::string &transport_name)
    : transport_instance(std::unique_ptr<Transport>(new Transport())) {
    transport_instance->open(transport_name);
  }

  void packet_in(const Packet &packet);
  void parser_start(const Packet &packet);
  void parser_extract(const Packet &packet);
  void deparser_start(const Packet &packet);
  void deparser_deparse(const Packet &packet);
  void cond_eval(const Packet &packet);
  void table_hit(const Packet &packet, const std::string &table_name);
  void table_miss(const Packet &packet);
  void action_execute(const Packet &packet);

public:
  // TODO: improve this super ugly test
  static EventLogger<Transport> *create_instance(const std::string transport_name) {
    static EventLogger<Transport> logger(transport_name);
    return &logger;
  }

private:
  std::unique_ptr<Transport> transport_instance;
};

#if defined ELOGGER_STDOUT
static EventLogger<TransportSTDOUT> *logger =
  EventLogger<TransportSTDOUT>::create_instance("");
#elif defined ELOGGER_NANOMSG
static EventLogger<TransportNanomsg> *logger =
  EventLogger<TransportNanomsg>::create_instance("ipc:///tmp/test_bm.ipc");
#else
static EventLogger<TransportNULL> *logger =
  EventLogger<TransportNULL>::create_instance("");
#endif

#define ELOG_PACKET_IN logger->packet_in
#define ELOG_TABLE_HIT logger->table_hit

#endif
