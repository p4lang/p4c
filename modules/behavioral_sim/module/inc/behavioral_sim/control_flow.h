#ifndef _BM_CONTROL_FLOW_H_
#define _BM_CONTROL_FLOW_H_

#include "phv.h"
#include "packet.h"

class ControlFlowNode {
public:
  virtual ControlFlowNode *operator()(const Packet &pkt, PHV *phv) = 0;
};

#endif
