#ifndef _BM_TRANSPORT_H_
#define _BM_TRANSPORT_H_

#include <string>
#include <mutex>

#include "nn.h"

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

#endif
