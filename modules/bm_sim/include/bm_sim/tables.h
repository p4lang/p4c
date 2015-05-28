#ifndef _BM_TABLES_H_
#define _BM_TABLES_H_

#include <memory>

#include "control_flow.h"
#include "named_p4object.h"
#include "match_tables.h"

class MatchActionTable : public ControlFlowNode, public NamedP4Object
{
public:
  MatchActionTable(const std::string &name, p4object_id_t id,
		   std::unique_ptr<MatchTableAbstract> match_table)
    : NamedP4Object(name, id),
      match_table(std::move(match_table)) { }

  const ControlFlowNode *operator()(Packet *pkt) const override {
    typedef MatchTableAbstract::ActionEntry ActionEntry;
    const ActionEntry &action_entry = match_table->lookup_action(*pkt);
    action_entry.action_fn(pkt);
    return action_entry.next_node;
  }

  MatchTableAbstract *get_match_table() { return match_table.get(); }

private:
  std::unique_ptr<MatchTableAbstract> match_table;
};

#endif
