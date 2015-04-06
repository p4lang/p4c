#include "bm_sim/pipeline.h"
#include "bm_sim/event_logger.h"

void
Pipeline::apply(Packet *pkt) {
  ELOGGER->pipeline_start(*pkt, *this);
  const ControlFlowNode *node = first_node;
  while(node) {
    node = (*node)(pkt); 
  }
  ELOGGER->pipeline_done(*pkt, *this);
}
