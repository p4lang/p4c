#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_SCOPE_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_SCOPE_H_

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/cstring.h"

namespace p4c::P4Tools::P4Smith {

struct Requirements {
    bool require_scalar{false};
    bool compile_time_known{false};
    bool no_methodcalls{false};
    bool not_zero{false};
    bool not_negative{false};
    bool byte_align_headers{false};
    int shift_width{8};
    Requirements()

        = default;
};

struct Constraints {
    bool const_header_stack_index{false};
    bool const_lshift_count{false};
    bool single_stage_actions{false};
    int max_phv_container_width{0};
    Constraints()

        = default;
};

struct Properties {
    bool width_unknown{false};
    bool has_methodcall{false};
    bool in_action{false};
    size_t depth = 0;
    // This means we are in a block that returns.
    // We need to return an expression with the specified type.
    const IR::Type *ret_type = nullptr;
    Properties() = default;
};

class P4Scope {
 public:
    /// This is a list of subscopes.
    static std::vector<IR::Vector<IR::Node> *> scope;

    /// Maintain a set of names we have already used to avoid duplicates.
    static std::set<cstring> usedNames;

    /// This is a map of usable lvalues we store to be used for references.
    static std::map<cstring, std::map<int, std::set<cstring>>> lvalMap;

    /// A subset of the lval map that includes rw values.
    static std::map<cstring, std::map<int, std::set<cstring>>> lvalMapRw;

    /// TODO: Maybe we can just remove tables from the declarations list?
    /// This is back-end specific.
    static std::set<const IR::P4Table *> callableTables;

    /// Structs that should not be initialized because they are incomplete.
    static std::set<cstring> notInitializedStructs;

    /// Properties that define the current state of the program.
    /// For example, when should a return expression must be returned in a block.
    static Properties prop;

    /// Back-end or node-specific restrictions.
    static Requirements req;

    /// This defines all constraints specific to various targets or back-ends.
    static Constraints constraints;

    P4Scope() = default;

    ~P4Scope() = default;

    static void addToScope(const IR::Node *n);
    static void startLocalScope();
    static void endLocalScope();

    static void addLval(const IR::Type *tp, cstring name, bool read_only = false);
    static bool checkLval(const IR::Type *tp, bool must_write = false);
    static cstring pickLval(const IR::Type *tp, bool must_write = false);
    static void deleteLval(const IR::Type *tp, cstring name);
    static std::set<cstring> getCandidateLvals(const IR::Type *tp, bool must_write = true);
    static bool hasWriteableLval(cstring typeKey);
    static std::optional<std::map<int, std::set<cstring>>> getWriteableLvalForTypeKey(
        cstring typeKey);

    static const IR::Type_Bits *pickDeclaredBitType(bool must_write = false);

    static const IR::Type_Declaration *getTypeByName(cstring name);

    // template to get all declarations
    // C++ is so shit... templates must be inlined to be generally usable.
    template <typename T>
    static std::vector<const T *> getDecls() {
        std::vector<const T *> ret;

        for (auto *subScope : scope) {
            for (const auto *node : *subScope) {
                if (const T *tmpObj = node->to<T>()) {
                    ret.push_back(tmpObj);
                }
            }
        }
        return ret;
    }

    static std::vector<const IR::Type_Declaration *> getFilteredDecls(std::set<cstring> filter);
    static std::set<const IR::P4Table *> *getCallableTables();
};
}  // namespace p4c::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_SCOPE_H_ */
