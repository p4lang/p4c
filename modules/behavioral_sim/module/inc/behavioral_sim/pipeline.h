#ifndef _BM_PIPELINE_H_
#define _BM_PIPELINE_H_

class Pipeline
{
public:
  Pipeline(const std::string &name, ControlFlowNode *first_node)
    : name(name), first_node(first_node) {}

  void apply(const Packet &pkt, PHV *phv) {
    const ControlFlowNode *node = first_node;
    while(node) {
      node = (*node)(pkt, phv); 
    }
  }

private:
  std::string name;
  ControlFlowNode *first_node;
};

#endif
