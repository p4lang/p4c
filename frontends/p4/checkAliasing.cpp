#include "checkAliasing.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

namespace {

// Canonical representation for Expressions which are
// left-values.
// We represent a set of such expressions as a forest.
// E.g., a.b, a.b[2].c, a.c, e.f
// The forest would be
// a
// ├─ b*
// │  └─ 2
// │     └─ c*
// └── c*
// e
// └── f*
// A star denotes a node that corresponds to an expression in the set.
// (This is the "argument" node of the TreeNode class.)
// A set of expression is aliased if there is a path in a tree that
// contains multiple stars.
// TODO: Replace this class with SymbolicValue
class CanonicalMemberExpression : public Inspector {
    const ReferenceMap* refMap;

    struct TreeNode {
        std::vector<const IR::Expression*> expressions;
        std::map<cstring, TreeNode*>    edges;
        TreeNode*                       parent = nullptr;
    };
    std::map<const IR::IDeclaration*, TreeNode*> roots;
    std::map<const IR::Expression*, TreeNode*>   representation;
    std::vector<const IR::Expression*>           allExpresions;

    bool preorder(const IR::Member* expression) override {
        visit(expression->expr);
        auto baseCanon = ::get(representation, expression->expr);
        CHECK_NULL(baseCanon);
        auto child = ::get(baseCanon->edges, expression->member.name);
        if (child == nullptr) {
            child = new TreeNode();
            baseCanon->edges.emplace(expression->member.name, child);
            child->parent = baseCanon;
        }
        representation.emplace(expression, child);
        return false;
    }

    bool preorder(const IR::ArrayIndex* expression) override {
        visit(expression->left);
        auto baseCanon = ::get(representation, expression->left);
        CHECK_NULL(baseCanon);
        cstring index;
        if (expression->right->is<IR::Constant>()) {
            auto cst = expression->right->to<IR::Constant>();
            index = cst->toString();
        } else {
            // This is imprecise, but we can't do much better
            index = "";
        }

        auto child = ::get(baseCanon->edges, index);
        if (child == nullptr) {
            child = new TreeNode();
            baseCanon->edges.emplace(index, child);
            child->parent = baseCanon;
        }
        representation.emplace(expression, child);
        return false;
    }

    bool preorder(const IR::PathExpression* expression) override {
        auto decl = refMap->getDeclaration(expression->path);
        auto canon = ::get(roots, decl);
        if (canon == nullptr) {
            canon = new TreeNode();
            roots.emplace(decl, canon);
        }
        representation.emplace(expression, canon);
        return false;
    }

 public:
    explicit CanonicalMemberExpression(const ReferenceMap* refMap) : refMap(refMap)
    { CHECK_NULL(refMap); }

    void add(const IR::Expression* expression) {
        expression->apply(*this);
        auto repr = ::get(representation, expression);
        CHECK_NULL(repr);
        repr->expressions.push_back(expression);
        allExpresions.push_back(expression);
    }

    void checkAliasing() const {
        for (auto e : allExpresions) {
            auto r = ::get(representation, e);
            CHECK_NULL(r);
            if (r->expressions.size() > 1)
                ::warning("%1% is aliased with %2%", r->expressions.front(), r->expressions.back());
            while (r->parent != nullptr) {
                r = r->parent;
                if (!r->expressions.empty())
                    ::warning("%1% is aliased with %2%", e, r->expressions.back());
            }
        }
    }
};

}  // namespace

bool CheckAliasing::preorder(const IR::MethodCallExpression* expression) {
    CanonicalMemberExpression cme(refMap);
    MethodCallDescription mcd(expression, refMap, typeMap);

    for (auto p : *mcd.substitution.getParameters()) {
        if (p->direction == IR::Direction::None ||
            p->direction == IR::Direction::In) continue;
        auto arg = mcd.substitution.lookup(p);
        cme.add(arg);
    }
    cme.checkAliasing();
    return false;
}

}  // namespace P4
