#ifndef _BM_TRANSPORT_H_
#define _BM_TRANSPORT_H_

#include <string>
#include <mutex>
#include <initializer_list>

#include "nn.h"

class TransportIface {
public:
  struct MsgBuf {
    char *buf;
    int len;
  };

public:
  virtual int open(const std::string &name) = 0;

  virtual int send(const std::string &msg) const = 0;
  virtual int send(const char *msg, int len) const = 0;

  virtual int send_msgs(const std::initializer_list<std::string> &msgs) const = 0;
  virtual int send_msgs(const std::initializer_list<MsgBuf> &msgs) const = 0;
};

class TransportNanomsg : public TransportIface {
public:
  TransportNanomsg();

  int open(const std::string &name) override;

  int send(const std::string &msg) const override;
  int send(const char *msg, int len) const override;

  int send_msgs(const std::initializer_list<std::string> &msgs) const override;
  int send_msgs(const std::initializer_list<MsgBuf> &msgs) const override;

private:
  nn::socket s;
};

class TransportSTDOUT : public TransportIface {
public:
  int open(const std::string &name) override;

  int send(const std::string &msg) const override;
  int send(const char *msg, int len) const override;

  int send_msgs(const std::initializer_list<std::string> &msgs) const override;
  int send_msgs(const std::initializer_list<MsgBuf> &msgs) const override;
};

class TransportNULL : public TransportIface {
public:
  int open(const std::string &name) override { return 0; }

  int send(const std::string &msg) const override { return 0; }
  int send(const char *msg, int len) const override { return 0; }

  int send_msgs(const std::initializer_list<std::string> &msgs) const override {
    return 0;
  }

  int send_msgs(const std::initializer_list<MsgBuf> &msgs) const override {
    return 0;
  }
};

#endif
