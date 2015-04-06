#ifndef _BM_CONTROL_FLOW_H_
#define _BM_CONTROL_FLOW_H_

#include "packet.h"

class ControlFlowNode {
public:
  virtual const ControlFlowNode *operator()(Packet *pkt) const = 0;
};

#endif
