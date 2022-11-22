#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_CONCOLIC_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_CONCOLIC_H_

#include <sys/types.h>

#include <functional>
#include <list>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/variant/variant.hpp>

#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/model.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools {

namespace P4Testgen {

/// TODO: This is a very ugly data structure. Essentially, you can store both state variables and
/// entire expression as keys. State variables can actually compared, expressions are always unique
/// keys. Using this map, you can look up particular state variables and check whether they actually
/// are present, but not expressions. The reason expressions need to be keys is that some times
/// entire expressions are mapped to a particular constant.
using ConcolicVariableMap =
    std::map<boost::variant<const StateVariable, const IR::Expression*>, const IR::Expression*>;

/// Encapsulates a set of concolic method implementations.
class ConcolicMethodImpls {
 private:
    using MethodImpl = std::function<void(
        cstring concolicMethodName, const IR::ConcolicVariable* var, const ExecutionState& state,
        const Model* completedModel, ConcolicVariableMap* resolvedConcolicVariables)>;

    std::map<cstring, std::map<uint, std::list<std::pair<std::vector<cstring>, MethodImpl>>>> impls;

    static bool matches(const std::vector<cstring>& paramNames,
                        const IR::Vector<IR::Argument>* args);

 public:
    using ImplList = std::list<std::tuple<cstring, std::vector<cstring>, MethodImpl>>;

    explicit ConcolicMethodImpls(const ImplList& implList);

    bool exec(cstring concolicMethodName, const IR::ConcolicVariable* var,
              const ExecutionState& state, const Model* completedModel,
              ConcolicVariableMap* resolvedConcolicVariables) const;

    void add(const ImplList& implList);
};

class ConcolicResolver : public Inspector {
 public:
    explicit ConcolicResolver(const Model* completedModel, const ExecutionState& state,
                              const ConcolicMethodImpls* concolicMethodImpls);

    ConcolicResolver(const Model* completedModel, const ExecutionState& state,
                     const ConcolicMethodImpls* concolicMethodImpls,
                     ConcolicVariableMap resolvedConcolicVariables);

    const ConcolicVariableMap* getResolvedConcolicVariables() const;

 private:
    bool preorder(const IR::ConcolicVariable* var) override;

    /// Execution state may be used by concolic implementations to access specific state.
    const ExecutionState& state;

    /// The completed model is queried to produce a (random) assignment for concolic inputs.
    gsl::not_null<const Model*> completedModel;

    /// A map of the concolic variables and the assertion associated with the variable. These
    /// assertions are used to add constraints to the solver.
    ConcolicVariableMap resolvedConcolicVariables;

    /// A pointer to the list of implemented concolic methods. This is assembled by the testgen
    /// targets.
    gsl::not_null<const ConcolicMethodImpls*> concolicMethodImpls;
};

class Concolic {
 public:
    static const ConcolicMethodImpls::ImplList* getCoreConcolicMethodImpls();
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_CONCOLIC_H_ */
