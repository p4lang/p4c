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

public:
  template <typename MT>
  static std::unique_ptr<MatchActionTable> create_match_action_table(
    const std::string &match_type,
    const std::string &name, p4object_id_t id,
    size_t size, const MatchKeyBuilder &match_key_builder,
    bool with_counters
  ) {
    static_assert(std::is_base_of<MatchTableAbstract, MT>::value,
		  "incorrect template, needs to be a subclass of MatchTableAbstract");
    std::unique_ptr<MT> match_table = MT::create_match_table(
      match_type, name, id, size, match_key_builder, with_counters
    );

    return std::unique_ptr<MatchActionTable>(
      new MatchActionTable(name, id, std::move(match_table))
    );
  }

private:
  std::unique_ptr<MatchTableAbstract> match_table;
};

#endif
