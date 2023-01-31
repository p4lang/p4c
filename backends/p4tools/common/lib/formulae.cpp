#include "backends/p4tools/common/lib/formulae.h"

#include "ir/id.h"
#include "lib/cstring.h"

namespace P4Tools {

/* =============================================================================================
 *  StateVariable
 * ============================================================================================= */

StateVariable::StateVariable(const IR::Expression *expr)
    : AbstractRepCheckedNode(checkedTo<IR::Member>(expr), "state variable") {}

bool StateVariable::repOk(const IR::Expression *expr) {
    // Only members can be state variables.
    const auto *member = expr->to<IR::Member>();
    if (member == nullptr) {
        return false;
    }

    // A member is a state variable if it is qualified by a PathExpression or if its qualifier is a
    // state variable.
    return member->expr->is<IR::PathExpression>() || repOk(member->expr);
}

bool StateVariable::operator<(const StateVariable &other) const {
    return compare(node.get(), other.node.get()) < 0;
}

bool StateVariable::operator==(const StateVariable &other) const {
    // Delegate to IR's notion of equality.
    return *node == *other.node;
}

int StateVariable::compare(const IR::Expression *e1, const IR::Expression *e2) {
    if (const auto *m1 = e1->to<IR::Member>()) {
        return compare(m1, e2);
    }
    if (const auto *p1 = e1->to<IR::PathExpression>()) {
        return compare(p1, e2);
    }
    BUG("Not a valid StateVariable: %1%", e1);
}

int StateVariable::compare(const IR::Member *m1, const IR::Expression *e2) {
    if (const auto *m2 = e2->to<IR::Member>()) {
        return compare(m1, m2);
    }
    if (e2->is<IR::PathExpression>()) {
        return 1;
    }
    BUG("Not a valid StateVariable: %1%", e2);
}

int StateVariable::compare(const IR::Member *m1, const IR::Member *m2) {
    auto result = compare(m1->expr, m2->expr);
    if (result != 0) {
        return result;
    }
    if (m1->member.name < m2->member.name) {
        return -1;
    }
    if (m1->member.name > m2->member.name) {
        return 1;
    }
    return 0;
}

int StateVariable::compare(const IR::PathExpression *p1, const IR::Expression *e2) {
    if (const auto *p2 = e2->to<IR::PathExpression>()) {
        return compare(p1, p2);
    }
    if (e2->is<IR::Member>()) {
        return -1;
    }
    BUG("Not a valid StateVariable: %1%", e2);
}

int StateVariable::compare(const IR::PathExpression *p1, const IR::PathExpression *p2) {
    if (p1->path->name.name < p2->path->name.name) {
        return -1;
    }
    if (p1->path->name.name > p2->path->name.name) {
        return 1;
    }
    return 0;
}

}  // namespace P4Tools
