#ifndef _BM_SWITCH_H_
#define _BM_SWITCH_H_

#include <memory>
#include <string>
#include <fstream>

#include "P4Objects.h"
#include "queue.h"
#include "packet.h"
#include "runtime_interface.h"

class Switch : public RuntimeInterface {
public:
  Switch();

  void init_objects(const std::string &json_path);

  P4Objects *get_p4objects() { return p4objects.get(); }

  virtual int receive(int port_num, const char *buffer, int len) = 0;

  virtual void start_and_return() = 0;

protected:
  std::unique_ptr<P4Objects> p4objects;
};

#endif
