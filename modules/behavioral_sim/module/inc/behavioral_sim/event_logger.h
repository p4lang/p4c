#ifndef _BM_EVENT_LOGGER_H_
#define _BM_EVENT_LOGGER_H_

#include <string>
#include <memory>

class TransportIface {
public:
  virtual int open(const std::string &name) = 0;

  virtual int send(const std::string &msg) const = 0;
  virtual int send(const char *msg, int len) const = 0;
};

class TransportNanomsg : public TransportIface {
public:
  int open(const std::string &name) override;

  int send(const std::string &msg) const override;
  int send(const char *msg, int len) const override;
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

  void log_parser_start();
  void log_parser_extract();
  void log_deparser_start();
  void log_deparser_deparse();
  void log_cond_eval();
  void log_table_hit();
  void log_table_miss();
  void log_action_execute();

public:
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
  EventLogger<TransportNanomsg>::create_instance("");
#else
static EventLogger<TransportNULL> *logger =
  EventLogger<TransportNULL>::create_instance("");
#endif

#define ELOG_TABLE_HIT logger->log_table_hit

#endif
