#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_P4STATE_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_P4STATE_H_

#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/ir.h"

namespace P4Tools {

namespace P4Smith {

class P4State {
 public:
    P4State() {}
    static IR::IndexedVector<IR::ParserState> state_list;

    ~P4State() {}

    static IR::MethodCallStatement *gen_hdr_extract(IR::Member *pkt_call, IR::Expression *mem);
    static void gen_hdr_union_extract(IR::IndexedVector<IR::StatOrDecl> &components,
                                      const IR::Type_HeaderUnion *hdru, IR::ArrayIndex *arr_ind,
                                      IR::Member *pkt_call);

    static IR::ParserState *gen_start_state();

    static IR::ParserState *gen_hdr_states();
    static void gen_state(cstring name);
    static void build_parser_tree();
    static IR::IndexedVector<IR::ParserState> get_states() { return state_list; }
};  // class p4State
}  // namespace P4Smith

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_P4STATE_H_ */
