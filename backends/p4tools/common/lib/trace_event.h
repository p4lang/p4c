#ifndef BACKENDS_P4TOOLS_COMMON_LIB_TRACE_EVENT_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_TRACE_EVENT_H_

#include <iosfwd>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "ir/visitor.h"
#include "lib/castable.h"

namespace P4Tools {

/// An event in a trace of the execution of a P4 program.
class TraceEvent : public ICastable {
 private:
    friend std::ostream &operator<<(std::ostream &os, const TraceEvent &event);

 public:
    virtual ~TraceEvent() = default;
    TraceEvent();

    /// Substitutes state variables in the body of this trace event for their symbolic value in the
    /// given symbolic environment. Variables that are unbound by the given environment are left
    /// untouched.
    [[nodiscard]] virtual const TraceEvent *subst(const SymbolicEnv &env) const;

    /// Applies the given IR transform to the expressions in this trace event.
    virtual const TraceEvent *apply(Transform &visitor) const;

    /// Evaluates expressions in the body of this trace event for their concrete value in the given
    /// model. A BUG occurs if there are any variables that are unbound by the given model.
    [[nodiscard]] virtual const TraceEvent *evaluate(const Model &model, bool doComplete) const;

 protected:
    /// Prints this trace event to the given ostream.
    virtual void print(std::ostream &) const = 0;

    DECLARE_TYPEINFO(TraceEvent);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_TRACE_EVENT_H_ */
