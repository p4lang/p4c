#ifndef _BM_EVENT_LOGGER_H_
#define _BM_EVENT_LOGGER_H_

#include <string>
#include <memory>

#include "nn.h"

#include "packet.h"
#include "phv.h"
#include "pipeline.h"
#include "tables.h"

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


/* Forward declarations of P4 object classes. This is ugly, but:
   1) I don't have to worry about circular dependencies
   2) If I decide to switch from id to name for msgs, I won't have to modify the
   EventLogger interface */
class Parser;
class Deparser;
class MatchTable;
class ActionFn;
class Conditional;

template <typename Transport>
class EventLogger {
public:
  EventLogger(const std::string &transport_name)
    : transport_instance(std::unique_ptr<Transport>(new Transport())) {
    transport_instance->open(transport_name);
  }

  // we need the ingress / egress ports, but they are part of the Packet
  void packet_in(const Packet &packet);
  void packet_out(const Packet &packet);

  void parser_start(const Packet &packet, const Parser &parser);
  void parser_done(const Packet &packet, const Parser &parser);
  void parser_extract(const Packet &packet, header_id_t header);

  void deparser_start(const Packet &packet, const Deparser &deparser);
  void deparser_done(const Packet &packet, const Deparser &deparser);
  void deparser_emit(const Packet &packet, header_id_t header);

  void pipeline_start(const Packet &packet, const Pipeline &pipeline);
  void pipeline_done(const Packet &packet, const Pipeline &pipeline);

  void condition_eval(const Packet &packet,
		      const Conditional &cond, bool result);
  void table_hit(const Packet &packet,
		 const MatchTable &table, const MatchEntry &entry);
  void table_miss(const Packet &packet, const MatchTable &table);

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

#define ELOGGER logger

#endif
