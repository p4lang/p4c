#include "backends/p4tools/modules/testgen/lib/continuation.h"

#include <variant>
#include <vector>

#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/id.h"
#include "ir/visitor.h"
#include "lib/exceptions.h"

namespace P4Tools::P4Testgen {

Continuation::Return::Return(const IR::Node *expr) : expr(expr) {}

bool Continuation::Return::operator==(const Continuation::Return &other) const {
    return expr ? other.expr && **expr == **other.expr : !other.expr;
}

Continuation::PropertyUpdate::PropertyUpdate(cstring propertyName, PropertyValue property)
    : propertyName(propertyName), property(property) {}

bool Continuation::PropertyUpdate::operator==(const Continuation::PropertyUpdate &other) const {
    return propertyName == other.propertyName && property == other.property;
}

Continuation::Guard::Guard(const IR::Expression *cond) : cond(cond) {}

bool Continuation::Guard::operator==(const Continuation::Guard &other) const {
    return *cond == *other.cond;
}

bool Continuation::Body::empty() const { return cmds.empty(); }

const Continuation::Command Continuation::Body::next() const {
    if (!cmds.empty()) {
        return cmds.front();
    }

    BUG("Empty continuation body");
}

void Continuation::Body::push(Command cmd) { cmds.emplace_front(cmd); }

void Continuation::Body::pop() {
    BUG_CHECK(!cmds.empty(), "Attempted to pop an empty command stack");
    cmds.pop_front();
}

void Continuation::Body::clear() { cmds.clear(); }

bool Continuation::Body::operator==(const Continuation::Body &other) const {
    return cmds == other.cmds;
}

Continuation::Body::Body(std::initializer_list<Command> cmds) : cmds(cmds) {}

Continuation::Body::Body(const std::vector<Command> &cmds) {
    for (auto it = cmds.rbegin(); it != cmds.rend(); ++it) {
        push(*it);
    }
}

/// This isn't a full-fledged capture-avoiding substitution, but should be good enough for our
/// needs. In particular, since the variables being substituted are freshly/uniquely generated, we
/// shouldn't have to worry about variable capture, and our continuations are created in a way that
/// we shouldn't have to worry about accidentally replacing lvalues.
class VariableSubstitution : public Transform {
 private:
    /// The variable being substituted.
    const IR::PathExpression *var;

    /// The expression being substituted for. Expressions in the metalanguage include P4
    /// non-expressions, so this expression does not necessarily need to be an instance of
    /// IR::Expression.
    const IR::Node *expr;

 public:
    const IR::Node *postorder(IR::PathExpression *pathExpr) override {
        if (*var == *pathExpr) {
            return expr;
        }
        return pathExpr;
    }

    /// Creates a pass that will substitute @expr for @var.
    VariableSubstitution(const IR::PathExpression *var, const IR::Node *expr)
        : var(var), expr(expr) {}
};

Continuation::Body Continuation::apply(std::optional<const IR::Node *> value_opt) const {
    BUG_CHECK(!(value_opt && !parameterOpt),
              "Supplied a value to a continuation with no parameters.");
    BUG_CHECK(!(!value_opt && parameterOpt),
              "Continuation %1% has a parameter of type %2%, but no argument provided.",
              (*parameterOpt), (*parameterOpt)->type->node_type_name());

    // No need to substitute if this continuation is parameterless.
    if (!parameterOpt) {
        return body;
    }

    const auto *paramType = (*parameterOpt)->type;
    const IR::Type *argType = nullptr;
    if (const auto *valueExpr = (*value_opt)->to<IR::Expression>()) {
        argType = valueExpr->type;
        // We step into an action enum type when resolving switch expressions. However, we return a
        // bit of unknown width. So, for this particular continuation, we have to take on the type
        // of the argument.
        // TODO: Resolve this in a cleaner fashion. Maybe we should not step at all?
        if (paramType->is<IR::Type_ActionEnum>()) {
            argType = paramType;
            auto clone = valueExpr->clone();
            clone->type = paramType;
            value_opt = clone;
        }

    } else if ((*value_opt)->is<IR::ParserState>()) {
        argType = IR::Type_State::get();
    } else {
        BUG("Unexpected node passed to continuation: %1%", *value_opt);
    }

    BUG_CHECK(
        paramType->equiv(*argType),
        "Continuation %1% has parameter of type %2%, but was given an argument %3% of type %4%",
        (*parameterOpt), paramType, *value_opt, argType);

    // Create a copy of this continuation's body, with the value substituted for the continuation's
    // parameter.
    Body result;

    struct SubstVisitor {
        VariableSubstitution subst;

        Command operator()(const IR::Node *node) { return node->apply(subst); }

        Command operator()(const TraceEvent *event) { return event->apply(subst); }

        Command operator()(Return ret) {
            if (!ret.expr) {
                return ret;
            }
            return Return{(*ret.expr)->apply(subst)};
        }

        Command operator()(Exception e) { return e; }

        Command operator()(PropertyUpdate p) { return p; }

        Command operator()(Guard g) { return Guard{g.cond->apply(subst)}; }

        /// Creates a visitor for Commands that substitutes the given value for the given
        /// parameter.
        SubstVisitor(const IR::PathExpression *param, const IR::Node *value)
            : subst(param, value) {}
    } subst(*parameterOpt, *value_opt);

    for (const auto &cmd : body.cmds) {
        result.cmds.push_back(std::visit(subst, cmd));
    }

    return result;
}

const Continuation::Parameter *Continuation::genParameter(const IR::Type *type, cstring name,
                                                          const NamespaceContext *ctx) {
    auto varName = ctx->genName(name, '*');
    return new Parameter(new IR::PathExpression(type, new IR::Path(varName)));
}

}  // namespace P4Tools::P4Testgen
