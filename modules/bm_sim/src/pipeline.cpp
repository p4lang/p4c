#include "bm_sim/pipeline.h"
#include "bm_sim/event_logger.h"

void
Pipeline::apply(const Packet &pkt, PHV *phv) {
  ELOGGER->pipeline_start(pkt, *this);
  const ControlFlowNode *node = first_node;
  while(node) {
    node = (*node)(pkt, phv); 
  }
  ELOGGER->pipeline_done(pkt, *this);
}
