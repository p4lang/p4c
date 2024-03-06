#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_SCOPE_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_SCOPE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/ir.h"

namespace P4Tools {

namespace P4Smith {

struct Requirements {
    bool require_scalar;
    bool compile_time_known;
    bool no_methodcalls;
    bool not_zero;
    bool not_negative;
    bool byte_align_headers;
    int shift_width;
    Requirements()
        : require_scalar(false),
          compile_time_known(false),
          no_methodcalls{false},
          not_zero(false),
          not_negative(false),
          byte_align_headers(false),
          shift_width(8) {}
};

struct Constraints {
    bool const_header_stack_index;
    bool const_lshift_count;
    bool single_stage_actions;
    int max_phv_container_width;
    Constraints()
        : const_header_stack_index(false),
          const_lshift_count(false),
          single_stage_actions(false),
          max_phv_container_width(0) {}
};

struct Properties {
    bool width_unknown;
    bool has_methodcall;
    bool in_action;
    size_t depth = 0;
    // This means we are in a block that returns.
    // We need to return an expression with the specified type.
    const IR::Type *ret_type = nullptr;
    Properties() : width_unknown(false), has_methodcall{false}, in_action{false} {}
};

class P4Scope {
 public:
    /// This is a list of subscopes.
    static std::vector<IR::Vector<IR::Node> *> scope;

    /// Maintain a set of names we have already used to avoid duplicates.
    static std::set<cstring> used_names;

    /// This is a map of usable lvalues we store to be used for references.
    static std::map<cstring, std::map<int, std::set<cstring>>> lval_map;

    /// A subset of the lval map that includes rw values.
    static std::map<cstring, std::map<int, std::set<cstring>>> lval_map_rw;

    /// TODO: Maybe we can just remove tables from the declarations list?
    /// This is back-end specific.
    static std::set<const IR::P4Table *> callable_tables;

    /// Structs that should not be initialized because they are incomplete.
    static std::set<cstring> not_initialized_structs;

    /// Properties that define the current state of the program.
    /// For example, when should a return expression must be returned in a block.
    static Properties prop;

    /// Back-end or node-specific restrictions.
    static Requirements req;

    /// This defines all constraints specific to various targets or back-ends.
    static Constraints constraints;

    P4Scope() {}

    ~P4Scope() {}

    static void add_to_scope(const IR::Node *n);
    static void start_local_scope();
    static void end_local_scope();

    static void add_lval(const IR::Type *tp, cstring name, bool read_only = false);
    static bool check_lval(const IR::Type *tp, bool must_write = false);
    static cstring pick_lval(const IR::Type *tp, bool must_write = false);
    static IR::Expression *pick_lval_or_slice(const IR::Type *tp);
    static void delete_lval(const IR::Type *tp, cstring name);
    static std::set<cstring> get_candidate_lvals(const IR::Type *tp, bool must_write = true);

    static IR::Type_Bits *pick_declared_bit_type(bool must_write = false);

    static const IR::Type_Declaration *get_type_by_name(cstring name);

    // template to get all declarations
    // C++ is so shit... templates must be inlined to be generally usable.
    template <typename T>
    static inline std::vector<const T *> get_decls() {
        std::vector<const T *> ret;

        for (auto sub_scope : scope) {
            for (auto node : *sub_scope) {
                if (const T *tmp_obj = node->to<T>()) {
                    ret.push_back(tmp_obj);
                }
            }
        }
        return ret;
    }

    static std::vector<const IR::Type_Declaration *> get_filtered_decls(std::set<cstring> filter);
    static std::set<const IR::P4Table *> *get_callable_tables();
};
}  // namespace P4Smith

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_SCOPE_H_ */
