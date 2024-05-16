#include "ir/ir.h"

namespace P4::IR {

namespace {

int compare(const Expression *e1, const Expression *e2);

int compare(const Member *m1, const Member *m2) {
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

int compare(const PathExpression *p1, const PathExpression *p2) {
    if (p1->path->name.name < p2->path->name.name) {
        return -1;
    }
    if (p1->path->name.name > p2->path->name.name) {
        return 1;
    }
    return 0;
}

int compare(const ArrayIndex *a1, const ArrayIndex *a2) {
    auto result = compare(a1->left, a2->left);
    if (result != 0) {
        return result;
    }
    const auto *a1Val = a1->right->to<Constant>();
    BUG_CHECK(
        a1Val != nullptr,
        "Value %1% is not a constant. Only constants are supported as part of a state variable.",
        a1->right);
    const auto *a2Val = a2->right->to<Constant>();
    BUG_CHECK(
        a2Val != nullptr,
        "Value %1% is not a constant. Only constants are supported as part of a state variable.",
        a2->right);
    if (a1Val->value < a2Val->value) {
        return -1;
    }
    if (a1Val->value > a2Val->value) {
        return 1;
    }
    return 0;
}

int compare(const Expression *e1, const Expression *e2) {
    // e1 is a Member.
    if (const auto *m1 = e1->to<Member>()) {
        if (const auto *m2 = e2->to<Member>()) {
            return IR::compare(m1, m2);
        }
        if (e2->is<PathExpression>()) {
            return 1;
        }
        if (e2->is<ArrayIndex>()) {
            return 1;
        }
    }
    // e1 is a PathExpression.
    if (const auto *p1 = e1->to<PathExpression>()) {
        if (const auto *p2 = e2->to<PathExpression>()) {
            return IR::compare(p1, p2);
        }
        if (e2->is<Member>()) {
            return -1;
        }
        if (e2->is<ArrayIndex>()) {
            return 1;
        }
    }
    // e1 is a ArrayIndex.
    if (const auto *a1 = e1->to<ArrayIndex>()) {
        if (const auto *a2 = e2->to<ArrayIndex>()) {
            return IR::compare(a1, a2);
        }
        if (e2->is<Member>()) {
            return -1;
        }
        if (e2->is<PathExpression>()) {
            return -1;
        }
    }
    BUG("Either %1% of type %2% or %3% of type %4% is not a valid StateVariable", e1,
        e1->node_type_name(), e2, e2->node_type_name());
}

}  // namespace

int StateVariable::compare(const Expression *e1, const Expression *e2) {
    return IR::compare(e1, e2);
}

namespace {

uint64_t hash(uint64_t seed, const Expression *expression);

uint64_t hash(uint64_t seed, const PathExpression *pathExpression) {
    return Util::hash_combine(seed, std::hash<cstring>()(pathExpression->path->name));
}

uint64_t hash(uint64_t seed, const ArrayIndex *arrayIndex) {
    const auto *arrayIndexVal = arrayIndex->right->to<Constant>();
    BUG_CHECK(
        arrayIndexVal != nullptr,
        "Value %1% is not a constant. Only constants are supported as part of a state variable.",
        arrayIndex->right);
    return IR::hash(Util::hash_combine(seed, std::hash<big_int>()(arrayIndexVal->value)),
                    arrayIndex->left);
}

uint64_t hash(uint64_t seed, const Member *member) {
    return IR::hash(Util::hash_combine(seed, std::hash<cstring>()(member->member)), member->expr);
}

uint64_t hash(uint64_t seed, const Expression *expression) {
    // expression is a Member.
    if (const auto *member = expression->to<Member>()) {
        return hash(seed, member);
    }
    // expression is a PathExpression.
    if (const auto *pathExpression = expression->to<PathExpression>()) {
        return hash(seed, pathExpression);
    }
    // expression is a ArrayIndex.
    if (const auto *arrayIndex = expression->to<ArrayIndex>()) {
        return hash(seed, arrayIndex);
    }
    BUG("Either %1% of type %2% is not a valid StateVariable", expression,
        expression->node_type_name());
}

}  // namespace

uint64_t StateVariable::hash() const { return IR::hash(0, ref); }

}  // namespace P4::IR
