/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lower.h"
#include "lib/gmputil.h"

namespace BMV2 {

// We make an effort to update the typeMap as we proceed
// since parent expression trees may need the information
// when processing in post-order.

const IR::Expression* LowerExpressions::shift(const IR::Operation_Binary* expression) const {
    auto rhs = expression->right;
    auto rhstype = typeMap->getType(rhs, true);
    if (rhstype->is<IR::Type_InfInt>()) {
        auto cst = rhs->to<IR::Constant>();
        mpz_class maxShift = Util::shift_left(1, LowerExpressions::maxShiftWidth);
        if (cst->value > maxShift)
            ::error("%1%: shift amount limited to %2% on this target", expression, maxShift);
    } else {
        BUG_CHECK(rhstype->is<IR::Type_Bits>(), "%1%: expected a bit<> type", rhstype);
        auto bs = rhstype->to<IR::Type_Bits>();
        if (bs->size > LowerExpressions::maxShiftWidth)
            ::error("%1%: shift amount limited to %2% bits on this target",
                    expression, LowerExpressions::maxShiftWidth);
    }
    auto ltype = typeMap->getType(getOriginal(), true);
    typeMap->setType(expression, ltype);
    return expression;
}

const IR::Node* LowerExpressions::postorder(IR::Neg* expression) {
    auto type = typeMap->getType(getOriginal(), true);
    auto zero = new IR::Constant(type, 0);
    auto sub = new IR::Sub(expression->srcInfo, zero, expression->expr);
    typeMap->setType(zero, type);
    typeMap->setType(sub, type);
    LOG3("Replaced " << expression << " with " << sub);
    return sub;
}

const IR::Node* LowerExpressions::postorder(IR::Cast* expression) {
    // handle bool <-> bit casts
    auto destType = typeMap->getType(getOriginal(), true);
    auto srcType = typeMap->getType(expression->expr, true);
    if (destType->is<IR::Type_Boolean>() && srcType->is<IR::Type_Bits>()) {
        auto zero = new IR::Constant(srcType, 0);
        auto cmp = new IR::Equ(expression->srcInfo, expression->expr, zero);
        typeMap->setType(cmp, destType);
        LOG3("Replaced " << expression << " with " << cmp);
        return cmp;
    } else if (destType->is<IR::Type_Bits>() && srcType->is<IR::Type_Boolean>()) {
        auto mux = new IR::Mux(expression->srcInfo, expression->expr,
                               new IR::Constant(destType, 1),
                               new IR::Constant(destType, 0));
        typeMap->setType(mux, destType);
        LOG3("Replaced " << expression << " with " << mux);
        return mux;
    }
    // This may be a new expression
    typeMap->setType(expression, destType);
    return expression;
}

const IR::Node* LowerExpressions::postorder(IR::Expression* expression) {
    // Just update the typeMap incrementally.
    auto type = typeMap->getType(getOriginal(), true);
    typeMap->setType(expression, type);
    return expression;
}

const IR::Node* LowerExpressions::postorder(IR::Slice* expression) {
    // This is in a RHS expression a[m:l]  ->  (cast)(a >> l)
    int h = expression->getH();
    int l = expression->getL();
    auto sh = new IR::Shr(expression->e0->srcInfo, expression->e0, new IR::Constant(l));
    auto e0type = typeMap->getType(expression->e0, true);
    typeMap->setType(sh, e0type);
    auto type = IR::Type_Bits::get(h - l + 1);
    auto result = new IR::Cast(expression->srcInfo, type, sh);
    typeMap->setType(result, type);
    LOG3("Replaced " << expression << " with " << result);
    return result;
}

const IR::Node* LowerExpressions::postorder(IR::Concat* expression) {
    // a ++ b  -> ((cast)a << sizeof(b)) | ((cast)b & mask)
    auto type = typeMap->getType(expression->right, true);
    auto resulttype = typeMap->getType(getOriginal(), true);
    BUG_CHECK(type->is<IR::Type_Bits>(), "%1%: expected a bitstring got a %2%",
              expression->right, type);
    BUG_CHECK(resulttype->is<IR::Type_Bits>(), "%1%: expected a bitstring got a %2%",
              expression->right, type);
    unsigned sizeofb = type->to<IR::Type_Bits>()->size;
    unsigned sizeofresult = resulttype->to<IR::Type_Bits>()->size;
    auto cast0 = new IR::Cast(expression->left->srcInfo, resulttype, expression->left);
    auto cast1 = new IR::Cast(expression->right->srcInfo, resulttype, expression->right);

    auto sh = new IR::Shl(cast0->srcInfo, cast0, new IR::Constant(sizeofb));
    mpz_class m = Util::maskFromSlice(sizeofb, 0);
    auto mask = new IR::Constant(expression->right->srcInfo,
                                 IR::Type_Bits::get(sizeofresult), m, 16);
    auto and0 = new IR::BAnd(expression->right->srcInfo, cast1, mask);
    auto result = new IR::BOr(expression->srcInfo, sh, and0);
    typeMap->setType(cast0, resulttype);
    typeMap->setType(cast1, resulttype);
    typeMap->setType(result, resulttype);
    typeMap->setType(sh, resulttype);
    typeMap->setType(and0, resulttype);
    LOG3("Replaced " << expression << " with " << result);
    return result;
}

/////////////////////////////////////////////////////////////

namespace {

struct VariableWriters {
    std::set<const IR::AssignmentStatement*> writers;
    VariableWriters() = default;
    explicit VariableWriters(const IR::AssignmentStatement* writer)
    { writers.emplace(writer); }
    VariableWriters* join(const VariableWriters* other) const {
        auto result = new VariableWriters();
        result->writers = writers;
        for (auto e : other->writers)
            result->writers.emplace(e);
        return result;
    }
    // Non-null only if there is exaclty one writer statement.
    // Return the RHS of the assignment
    const IR::Expression* substitution() const {
        if (writers.size() != 1)
            return nullptr;
        auto first = *writers.begin();
        return first->right;
    }
};

struct VariableDefinitions {
    std::map<cstring, const VariableWriters*> writers;
    VariableDefinitions(const VariableDefinitions& other) = default;
    VariableDefinitions() = default;
    VariableDefinitions* clone() const {
        return new VariableDefinitions(*this);
    }
    VariableDefinitions* join(const VariableDefinitions* other) const {
        auto result = clone();
        for (auto e : other->writers) {
            auto &prev = result->writers[e.first];
            prev = prev ? prev->join(e.second) : e.second;
        }
        return result;
    }
    void declare(const IR::Declaration_Variable* decl) {
        writers.emplace(decl->getName().name, new VariableWriters());
    }
    const VariableWriters* getWriters(const IR::Path* path) {
        if (path->absolute)
            return nullptr;
        return ::get(writers, path->name.name);
    }
    VariableDefinitions* setDefinition(const IR::Path* path,
                                       const IR::AssignmentStatement* statement) {
        auto w = getWriters(path);
        if (w == nullptr)
            // Path does not represent a variable
            return this;
        auto result = clone();
        result->writers[path->name.name] = new VariableWriters(statement);
        return result;
    }
};

struct PathSubstitutions {
    std::map<const IR::PathExpression*, const IR::Expression*> definitions;
    std::set<const IR::AssignmentStatement*> haveUses;
    PathSubstitutions() = default;
    void add(const IR::PathExpression* path, const IR::Expression* expression) {
        definitions.emplace(path, expression);
        LOG3("Will substitute " << dbp(path) << " with " << expression);
    }
    const IR::Expression* get(const IR::PathExpression* path) const {
        return ::get(definitions, path);
    }
    void foundUses(const VariableWriters* writers) {
        for (auto w : writers->writers)
            haveUses.emplace(w);
    }
    void foundUses(const IR::AssignmentStatement* statement) {
        haveUses.emplace(statement);
    }
    bool hasUses(const IR::AssignmentStatement* statement) const {
        return haveUses.find(statement) != haveUses.end();
    }
};

// See the SimpleCopyProp pass below for the context in which this
// analysis is run.  We take advantage that some more complex code
// patterns have already been eliminated.
class Accesses : public Inspector {
    PathSubstitutions* substitutions;
    VariableDefinitions* currentDefinitions;

    bool notSupported(const IR::Node* node) {
        ::error("%1%: not supported in checksum update control", node);
        return false;
    }

 public:
    explicit Accesses(PathSubstitutions* substitutions): substitutions(substitutions) {
        CHECK_NULL(substitutions); setName("Accesses");
        currentDefinitions = new VariableDefinitions();
    }

    bool preorder(const IR::Declaration_Variable* decl) override {
        // we assume all variable declarations are at the beginning
        currentDefinitions->declare(decl);
        return false;
    }

    // This is only invoked for read expressions
    bool preorder(const IR::PathExpression* expression) override {
        auto writers = currentDefinitions->getWriters(expression->path);
        if (writers != nullptr) {
            if (auto s = writers->substitution())
                substitutions->add(expression, s);
            else
                substitutions->foundUses(writers);
        }
        return false;
    }

    bool preorder(const IR::AssignmentStatement* statement) override {
        visit(statement->right);
        if (statement->left->is<IR::PathExpression>()) {
            auto pe = statement->left->to<IR::PathExpression>();
            currentDefinitions = currentDefinitions->setDefinition(pe->path, statement);
        } else {
            substitutions->foundUses(statement);
        }
        return false;
    }

    bool preorder(const IR::IfStatement* statement) override {
        visit(statement->condition);
        auto defs = currentDefinitions->clone();
        visit(statement->ifTrue);
        auto afterTrue = currentDefinitions;
        if (statement->ifFalse != nullptr) {
            currentDefinitions = defs;
            visit(statement->ifFalse);
            currentDefinitions = afterTrue->join(currentDefinitions);
        } else {
            currentDefinitions = defs->join(afterTrue);
        }
        return false;
    }

    bool preorder(const IR::SwitchStatement* statement) override
    { return notSupported(statement); }

    bool preorder(const IR::P4Action* action) override
    { return notSupported(action); }

    bool preorder(const IR::P4Table* table) override
    { return notSupported(table); }

    bool preorder(const IR::ReturnStatement* statement) override
    { return notSupported(statement); }

    bool preorder(const IR::ExitStatement* statement) override
    { return notSupported(statement); }
};

class Replace : public Transform {
    const PathSubstitutions* substitutions;
 public:
    explicit Replace(const PathSubstitutions* substitutions): substitutions(substitutions) {
        CHECK_NULL(substitutions); setName("Accesses"); }

    const IR::Node* postorder(IR::AssignmentStatement* statement) override {
        if (!substitutions->hasUses(getOriginal<IR::AssignmentStatement>()))
            return new IR::EmptyStatement();
        return statement;
    }

    const IR::Node* postorder(IR::PathExpression* expression) override {
        auto repl = substitutions->get(getOriginal<IR::PathExpression>());
        if (repl != nullptr) {
            Replace rpl(substitutions);
            auto recurse = repl->apply(rpl);
            return recurse;
        }
        return expression;
    }
};

// This analysis is only executed on the control which performs
// checksum update computations.
//
// This is a simpler variant of copy propagation; it just finds
// patterns of the form:
// tmp = X;
// ...
// out = tmp;
// then it substitutes the definition into the use.
// The LocalCopyPropagation pass does not do this, because
// it won't consider replacing definitions where the RHS has side-effects.
// Since the only method call we accept in the checksum update block
// is a checksum unit "get" method (this is not checked here, but
// in the json code generator), we know that this method has no side-effects,
// so we can safely reorder calls to methods.
// Also, this is run after eliminating struct and tuple operations,
// so we know that all assignments operate on scalar values.
class SimpleCopyProp : public PassManager {
    PathSubstitutions substitutions;
 public:
    SimpleCopyProp() {
        setName("SimpleCopyProp");
        passes.push_back(new Accesses(&substitutions));
        passes.push_back(new Replace(&substitutions));
    }
};

}  // namespace

const IR::Node* FixupChecksum::preorder(IR::P4Control* control) {
    if (control->name == *updateBlockName) {
        SimpleCopyProp scp;
        return control->apply(scp);
    }
    return control;
}

////////////////////////////////////////////////////////////////////////////////////

namespace {

// Detect whether a Select expression is too complicated for BMv2
class ComplexExpression : public Inspector {
 public:
    bool isComplex = false;
    ComplexExpression() { setName("ComplexExpression"); }

    void postorder(const IR::PathExpression*) override {}
    void postorder(const IR::Member*) override {}
    void postorder(const IR::Literal*) override {}
    // all other expressions are complex
    void postorder(const IR::Expression*) override { isComplex = true; }
};

}  // namespace

const IR::Node*
RemoveExpressionsFromSelects::postorder(IR::SelectExpression* expression) {
    bool changes = true;
    auto vec = new IR::Vector<IR::Expression>();
    for (auto e : *expression->select->components) {
        ComplexExpression ce;
        (void)e->apply(ce);
        if (ce.isComplex) {
            changes = true;
            auto type = typeMap->getType(e, true)->getP4Type();
            auto name = refMap->newName("tmp");
            auto decl = new IR::Declaration_Variable(Util::SourceInfo(), IR::ID(name),
                                                     IR::Annotations::empty, type, nullptr);
            newDecls.push_back(decl);
            auto assign = new IR::AssignmentStatement(
                Util::SourceInfo(), new IR::PathExpression(name), e);
            assignments.push_back(assign);
            vec->push_back(new IR::PathExpression(name));
        } else {
            vec->push_back(e);
        }
    }

    if (changes)
        expression->select = new IR::ListExpression(expression->select->srcInfo, vec);
    return expression;
}

}  // namespace BMV2
