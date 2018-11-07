/*
Copyright 2016 VMware, Inc.

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

#include "frontends/p4/sideEffects.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/cloner.h"

namespace P4 {

namespace {

/** Data structure used for making the order of evaluation explicit for
 * sub-expressions in an expression.  An expression ```e``` will be represented
 * as a sequence of temporary declarations, followed by a sequence of
 * statements (mostly assignments to the temporaries, but which could also
 * include conditionals for short-circuit evaluation).
 */
struct EvaluationOrder {
    ReferenceMap* refMap;

    /// Output: An expression whose evaluation will produce the same result as
    /// the original one, but where side-effects have been factored out such
    /// that each statement produces at most one side-effect.
    const IR::Expression* final;

    // Declaration instead of Declaration_Variable so it can be more easily inserted
    // in the program IR.
    IR::IndexedVector<IR::Declaration> *temporaries;
    IR::IndexedVector<IR::StatOrDecl> *statements;

    explicit EvaluationOrder(ReferenceMap* refMap) :
            refMap(refMap), final(nullptr),
            temporaries(new IR::IndexedVector<IR::Declaration>()),
            statements(new IR::IndexedVector<IR::StatOrDecl>())
    { CHECK_NULL(refMap); }

    bool simple() const
    { return temporaries->empty() && statements->empty(); }

    cstring createTemporary(const IR::Type* type) {
        type = type->getP4Type();
        auto tmp = refMap->newName("tmp");
        auto decl = new IR::Declaration_Variable(IR::ID(tmp, nullptr), type);
        temporaries->push_back(decl);
        return tmp;
    }

    /** Add ```@varName = @expression``` to the vector of statements.
      *
      * @return A copy of the l-value expression created for varName.
      */
    const IR::Expression* addAssignment(
        Util::SourceInfo srcInfo,
        cstring varName,
        const IR::Expression* expression) {
        auto left = new IR::PathExpression(IR::ID(varName, nullptr));
        auto stat = new IR::AssignmentStatement(srcInfo, left, expression);
        statements->push_back(stat);
        auto result = left->clone();
        return result;
    }
};

/** @brief Replaces expressions containing constructor or method invocations
 * with temporaries.  Also unrolls short-circuit evaluation.
 *
 * The EvaluationOrder field accumulates lists of the temporaries introduced in
 * this way, as well as assignment statements that assign the original
 * expressions to the appropriate temporaries.
 *
 * The DoSimplifyExpressions pass will later insert the declarations and
 * assignments above the expression.
 *
 * @pre An up-to-date ReferenceMap and TypeMap.
 *
 * @post The RefenceMap and TypeMap are updated to reflect changes to the IR.
 * This includes extra information stored in the TypeMap, specifically
 * left-value-ness and compile-time-contant-ness.
 */
class DismantleExpression : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    EvaluationOrder *result;

    /// true when we are dismantling a left-value.
    bool leftValue;

    /// true when the caller does not want the result (i.e.,
    /// we are invoked from a MethodCallStatement).
    bool resultNotUsed;

    // catch-all case
    const IR::Node* postorder(IR::Expression* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto orig = getOriginal<IR::Expression>();
        auto type = typeMap->getType(orig, true);
        typeMap->setType(expression, type);
        if (typeMap->isLeftValue(orig))
            typeMap->setLeftValue(expression);
        if (typeMap->isCompileTimeConstant(orig))
            typeMap->setCompileTimeConstant(expression);
        result->final = expression;
        return result->final;
    }

    const IR::Node* preorder(IR::Literal* expression) override {
        result->final = expression;
        prune();
        return expression;
    }

    const IR::Node* preorder(IR::ArrayIndex* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        if (!SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
            result->final = expression;
        } else {
            visit(expression->left);
            auto left = result->final;
            CHECK_NULL(left);
            bool save = leftValue;
            leftValue = false;
            visit(expression->right);
            auto right = result->final;
            CHECK_NULL(right);
            leftValue = save;
            if (!right->is<IR::Constant>()) {
                auto indexType = typeMap->getType(expression->right, true);
                auto tmp = result->createTemporary(indexType);
                right = result->addAssignment(expression->srcInfo, tmp, right);
                typeMap->setType(right, indexType);
            }

            result->final = new IR::ArrayIndex(expression->srcInfo, left, right);
        }
        typeMap->setType(result->final, type);
        if (leftValue)
            typeMap->setLeftValue(result->final);
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::Member* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        if (!SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
            result->final = expression;
        } else {
            visit(expression->expr);
            auto left = result->final;
            CHECK_NULL(left);
            result->final = new IR::Member(expression->srcInfo, left, expression->member);
            typeMap->setType(result->final, type);
            if (leftValue)
                typeMap->setLeftValue(result->final);

            // Special case for table.apply().hit, which is not dismantled by
            // the MethodCallExpression.
            if (TableApplySolver::isHit(expression, refMap, typeMap)) {
                BUG_CHECK(type->is<IR::Type_Boolean>(), "%1%: not boolean", type);
                auto tmp = result->createTemporary(type);
                auto path = new IR::PathExpression(IR::ID(tmp, nullptr));
                auto stat = new IR::AssignmentStatement(path, result->final);
                result->statements->push_back(stat);
                result->final = path->clone();
                typeMap->setType(result->final, type);
            }
            prune();
            return result->final;
        }
        typeMap->setType(result->final, type);
        if (leftValue)
            typeMap->setLeftValue(result->final);
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::SelectExpression* expression) override {
        LOG3("Visiting " << dbp(expression));
        visit(expression->select);
        prune();
        result->final = expression;
        return expression;
    }

    const IR::Node* preorder(IR::Operation_Unary* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        visit(expression->expr);
        auto left = result->final;
        CHECK_NULL(left);
        auto clone = expression->clone();
        clone->expr = left;
        typeMap->setType(clone, type);
        result->final = clone;
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::Operation_Binary* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        if (!SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
            result->final = expression;
        } else {
            visit(expression->left);
            auto left = result->final;
            CHECK_NULL(left);
            visit(expression->right);
            auto right = result->final;
            auto clone = expression->clone();
            clone->left = left;
            clone->right = right;
            typeMap->setType(clone, type);
            auto tmp = result->createTemporary(type);
            auto path = result->addAssignment(expression->srcInfo, tmp, clone);
            result->final = path;
        }
        typeMap->setType(result->final, type);
        prune();
        return result->final;
    }

    const IR::Node* shortCircuit(IR::Operation_Binary* expression) {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        if (!SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
            result->final = expression;
        } else {
            visit(expression->left);
            auto cond = result->final;
            CHECK_NULL(cond);

            // e1 && e2
            // becomes roughly:
            // if (!simplify(e1))
            //    tmp = false;
            // else
            //    tmp = simplify(e2);

            bool land = expression->is<IR::LAnd>();
            auto constant = new IR::BoolLiteral(!land);
            auto tmp = result->createTemporary(type);
            auto ifTrue = new IR::AssignmentStatement(expression->srcInfo,
                new IR::PathExpression(IR::ID(tmp, nullptr)), constant);
            auto ifFalse = new IR::IndexedVector<IR::StatOrDecl>();

            auto save = result->statements;
            result->statements = ifFalse;
            visit(expression->right);
            auto path = result->addAssignment(expression->srcInfo, tmp, result->final);
            result->statements = save;
            if (land) {
                cond = new IR::LNot(cond);
                typeMap->setType(cond, type);
            }
            auto block = new IR::BlockStatement(*ifFalse);
            auto ifStatement = new IR::IfStatement(expression->srcInfo, cond, ifTrue, block);
            result->statements->push_back(ifStatement);
            result->final = path->clone();
        }
        typeMap->setType(result->final, type);
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::Mux* expression) override {
        // We always dismantle muxes - some architectures may not support them
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        visit(expression->e0);
        auto e0 = result->final;
        CHECK_NULL(e0);
        auto tmp = result->createTemporary(type);

        auto save = result->statements;
        auto ifTrue = new IR::IndexedVector<IR::StatOrDecl>();
        result->statements = ifTrue;
        visit(expression->e1);
        (void)result->addAssignment(expression->srcInfo, tmp, result->final);

        auto ifFalse = new IR::IndexedVector<IR::StatOrDecl>();
        result->statements = ifFalse;
        visit(expression->e2);
        auto path = result->addAssignment(expression->srcInfo, tmp, result->final);
        result->statements = save;

        auto ifStatement = new IR::IfStatement(
            e0, new IR::BlockStatement(*ifTrue), new IR::BlockStatement(*ifFalse));
        result->statements->push_back(ifStatement);
        result->final = path->clone();
        typeMap->setType(result->final, type);
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::LAnd* expression) override { return shortCircuit(expression); }
    const IR::Node* preorder(IR::LOr* expression) override { return shortCircuit(expression); }

    /// This class represents the path to a location.
    /// Given a struct S { bit a; bit b; } and a variable S x;
    /// a path can be x.a, or just x.  An array index is represented as a
    /// number (encoded as a string) or as "*", denoting an unknown index.
    struct LocationPath : public IHasDbPrint {
        const IR::IDeclaration* root;
        std::vector<cstring> path;

        explicit LocationPath(const IR::IDeclaration* root): root(root) { CHECK_NULL(root); }

        const LocationPath* append(cstring suffix) const {
            auto result = new LocationPath(root);
            result->path = path;
            result->path.push_back(suffix);
            return result;
        }

        /// True if this path is a prefix of other or the other way around
        bool isPrefix(const LocationPath* other) const {
            // Due to the structure of the P4 language, two distinct
            // declarations can never alias.
            if (root != other->root)
                return false;
            size_t len = std::min(path.size(), other->path.size());
            for (size_t i = 0; i < len; i++) {
                if (path.at(i) == "*" || other->path.at(i) == "*")
                    continue;
                if (path.at(i) != other->path.at(i))
                    return false;
            }
            return true;
        }

        void dbprint(std::ostream& out) const override {
            out << root->getName();
            for (auto p : path)
                out << "." << p;
        }
    };

    /// We represent a set of location set as a set of LocationPath
    /// objects.
    class SetOfLocations : public IHasDbPrint {
     public:
        std::set<const LocationPath*> paths;

        SetOfLocations() = default;
        explicit SetOfLocations(const LocationPath* path) {
            add(path);
        }
        explicit SetOfLocations(const SetOfLocations* set): paths(set->paths) {}

        void add(const LocationPath* path) { paths.emplace(path); }
        bool overlaps(const SetOfLocations* other) const {
            // Normally one of these sets has only one element, because
            // one of the two is a left-value, so this should be fast.
            for (auto s : paths) {
                for (auto so : other->paths) {
                    if (s->isPrefix(so))
                        return true;
                }
            }
            return false;
        }

        const SetOfLocations* join(const SetOfLocations* other) const {
            auto result = new SetOfLocations(this);
            for (auto p : other->paths)
                result->add(p);
            return result;
        }

        /// Append suffix to each location in the set
        const SetOfLocations* append(cstring suffix) const {
            auto result = new SetOfLocations();
            for (auto p : paths) {
                auto append = p->append(suffix);
                result->add(append);
            }
            return result;
        }

        void dbprint(std::ostream& out) const override {
            for (auto p : paths)
                out << p << std::endl;
        }
    };

    /// Computes the SetOfLocations read and written by an expression.
    /// This is invoked only for expressions that appear as arguments
    /// to method calls.
    class ReadsWrites : public Inspector {
        const ReferenceMap* refMap;
        std::map<const IR::Expression*, const SetOfLocations*> rw;

     public:
        explicit ReadsWrites(const ReferenceMap* refMap) : refMap(refMap)
        { setName("ReadsWrites"); }

        void postorder(const IR::Operation_Binary* expression) override {
            auto left = ::get(rw, expression->left);
            auto right = ::get(rw, expression->right);
            rw.emplace(expression, left->join(right));
        }

        void postorder(const IR::PathExpression* expression) override {
            auto decl = refMap->getDeclaration(expression->path);
            auto path = new LocationPath(decl);
            auto locs = new SetOfLocations(path);
            rw.emplace(expression, locs);
        }

        void postorder(const IR::Operation_Unary* expression) override {
            auto e = ::get(rw, expression->expr);
            rw.emplace(expression, e);
        }

        void postorder(const IR::Member* expression) override {
            auto e = ::get(rw, expression->expr);
            auto result = e->append(expression->member);
            rw.emplace(expression, result);
        }

        void postorder(const IR::ArrayIndex* expression) override {
            auto e = ::get(rw, expression->left);
            const SetOfLocations* result;
            if (expression->right->is<IR::Constant>()) {
                int index = expression->right->to<IR::Constant>()->asInt();
                result = e->append(Util::toString(index));
            } else {
                result = e->append("*");
            }
            rw.emplace(expression, result);
        }

        void postorder(const IR::Literal* expression) override {
            rw.emplace(expression, new SetOfLocations());
        }

        void postorder(const IR::TypeNameExpression* expression) override {
            rw.emplace(expression, new SetOfLocations());
        }

        void postorder(const IR::Operation_Ternary* expression) override {
            auto e0 = ::get(rw, expression->e0);
            auto e1 = ::get(rw, expression->e1);
            auto e2 = ::get(rw, expression->e2);
            rw.emplace(expression, e0->join(e1)->join(e2));
        }

        void postorder(const IR::MethodCallExpression* expression) override {
            // The only expression that can appear here is h.isValid();
            // The ReadsWrites analysis is not called for other methods that
            // have side-effects -- these are always copied into temporaries.
            BUG_CHECK(expression->method->is<IR::Member>(),
                      "%1%: expected isValid()", expression);
            auto member = expression->method->to<IR::Member>();
            BUG_CHECK(member->member == "isValid", "%1%: expected isValid()", expression);
            auto obj = member->expr;
            auto e = ::get(rw, obj);
            rw.emplace(expression, e->append("$valid"));
        }

        void postorder(const IR::ConstructorCallExpression* expression) override {
            const SetOfLocations* result = new SetOfLocations();
            for (auto e : *expression->arguments) {
                auto s = ::get(rw, e->expression);
                result = result->join(s);
            }
            rw.emplace(expression, result);
        }

        void postorder(const IR::StructInitializerExpression* expression) override {
            const SetOfLocations* result = new SetOfLocations();
            for (auto e : expression->components) {
                auto s = ::get(rw, e->expression);
                result = result->join(s);
            }
            rw.emplace(expression, result);
        }

        void postorder(const IR::ListExpression* expression) override {
            const SetOfLocations* result = new SetOfLocations();
            for (auto e : expression->components) {
                auto s = ::get(rw, e);
                result = result->join(s);
            }
            rw.emplace(expression, result);
        }

        const SetOfLocations* get(const IR::Expression* expression) {
            expression->apply(*this);
            auto result = ::get(rw, expression);
            CHECK_NULL(result);
            LOG3("SetOfLocations(" << expression << ")=" << result);
            return result;
        }
    };

    // Conservative alias analysis.  We implement this here because this pass
    // runs early in the front end, before enough information is present (eg.
    // def-use information) to do a precise alias analysis.
    bool mayAlias(const IR::Expression* left, const IR::Expression* right) const {
        ReadsWrites rw(refMap);
        auto llocs = rw.get(left);
        auto rlocs = rw.get(right);
        LOG3("Checking overlap between " << llocs << " and " << rlocs);
        return llocs->overlaps(rlocs);
    }

    /// Returns true if type is a header or a struct containing a header.
    /// (We don't care about stacks or unions.)
    bool containsHeaderType(const IR::Type* type) {
        if (type->is<IR::Type_Header>())
            return true;
        auto st = type->to<IR::Type_Struct>();
        if (st == nullptr)
            return false;
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f, true);
            if (containsHeaderType(ftype))
                return true;
        }
        return false;
    }

    const IR::Node* preorder(IR::MethodCallExpression* mce) override {
        BUG_CHECK(!leftValue, "%1%: method on left hand side?", mce);
        LOG3("Visiting " << dbp(mce));
        auto orig = getOriginal<IR::MethodCallExpression>();
        auto type = typeMap->getType(orig, true);
        if (!SideEffects::check(orig, refMap, typeMap)) {
            result->final = mce;
            return mce;
        }

        auto copyBack = new IR::IndexedVector<IR::StatOrDecl>();
        auto args = new IR::Vector<IR::Argument>();
        auto mi = MethodInstance::resolve(orig, refMap, typeMap);
        bool savelv = leftValue;
        bool savenu = resultNotUsed;
        resultNotUsed = false;

        // If a parameter is in this set then we use a temporary to
        // copy the corresponding argument.  We could always use
        // temporaries for arguments - it is always correct - but this
        // could entail the creation of "fat" temporaries that contain
        // large structs.  We want to avoid copying these large
        // structs if possible.
        std::set<const IR::Parameter*> useTemporary;

        bool anyOut = false;
        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            if (p->direction == IR::Direction::None)
                continue;
            if (p->hasOut())
                anyOut = true;
            auto arg = mi->substitution.lookup(p);
            // If an argument evaluation has side-effects then
            // always use a temporary to hold the argument value.
            if (SideEffects::check(arg->expression, refMap, typeMap)) {
                LOG3("Using temporary for " << dbp(mce) <<
                     " param " << dbp(p) << " arg side effect");
                useTemporary.emplace(p);
                continue;
            }
            // If the parameter contains header values and the
            // argument is a list expression or a struct initializer
            // then we also use a temporary.  This makes the job of
            // the SetHeaders pass later simpler (otherwise we have to
            // handle this case there).
            auto ptype = typeMap->getType(p, true);
            if (!containsHeaderType(ptype))
                continue;

            if (arg->expression->is<IR::ListExpression>() ||
                arg->expression->is<IR::StructInitializerExpression>()) {
                LOG3("Using temporary for " << dbp(mce) <<
                     " param " << dbp(p) << " assigning tuple to header");
                useTemporary.emplace(p);
                continue;
            }
        }

        if (anyOut) {
            // Check aliasing between all pairs of arguments where at
            // least one of them is out or inout.
            for (auto p1 : *mi->substitution.getParametersInArgumentOrder()) {
                auto arg1 = mi->substitution.lookup(p1);
                for (auto p2 : *mi->substitution.getParametersInArgumentOrder()) {
                    if (p2 == p1)
                        break;
                    if (!p1->hasOut() && !p2->hasOut())
                        continue;
                    if (useTemporary.find(p1) != useTemporary.end())
                        continue;
                    if (useTemporary.find(p2) != useTemporary.end())
                        continue;
                    auto arg2 = mi->substitution.lookup(p2);
                    if (mayAlias(arg1->expression, arg2->expression)) {
                        LOG3("Using temporary for " << dbp(mce) <<
                             " param " << dbp(p1) << " aliasing" << dbp(p2));
                        useTemporary.emplace(p1);
                        useTemporary.emplace(p2);
                        break;
                    }
                }
            }
        }

        visit(mce->method);
        auto method = result->final;

        ClonePathExpressions cloner;  // a cheap version of deep copy
        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            auto arg = mi->substitution.lookup(p);
            if (p->direction == IR::Direction::None) {
                args->push_back(arg);
                continue;
            }

            bool useTemp = useTemporary.count(p) != 0;
            LOG3("Transforming " << dbp(arg) << " for " << dbp(p) <<
                 (useTemp ? " with " : " without ") << "temporary");

            if (p->direction == IR::Direction::In)
                leftValue = false;
            else
                leftValue = true;
            auto paramtype = typeMap->getType(p, true);
            const IR::Expression* argValue;
            visit(arg);  // May mutate arg!  Recursively simplifies arg.
            auto newarg = result->final;
            CHECK_NULL(newarg);

            if (useTemp) {
                // declare temporary variable
                auto tmp = refMap->newName("tmp");
                argValue = new IR::PathExpression(IR::ID(tmp, nullptr));
                auto decl = new IR::Declaration_Variable(
                    IR::ID(tmp, nullptr), paramtype->getP4Type());
                result->temporaries->push_back(decl);
                if (p->direction != IR::Direction::Out) {
                    auto clone = argValue->clone();
                    auto stat = new IR::AssignmentStatement(clone, newarg);
                    LOG3(clone << " = " << newarg);
                    result->statements->push_back(stat);
                    typeMap->setType(clone, paramtype);
                    typeMap->setLeftValue(clone);
                }
            } else {
                argValue = newarg;
            }
            if (leftValue && useTemp) {
                auto assign = new IR::AssignmentStatement(
                    cloner.clone<IR::Expression>(newarg),
                    cloner.clone<IR::Expression>(argValue));
                copyBack->push_back(assign);
                LOG3("Will copy out value " << dbp(assign));
            }
            args->push_back(new IR::Argument(arg->name, argValue));
        }
        leftValue = savelv;
        resultNotUsed = savenu;

        // Special handling for table.apply(...).X; we cannot generate
        // a temporary for the method call tmp = table.apply(), since
        // we cannot write down the type of tmp.  So we don't
        // dismantle these expressions.
        bool tbl_apply = false;
        if (auto mmbr = getParent<IR::Member>()) {
            auto tbl = TableApplySolver::isActionRun(mmbr, refMap, typeMap);
            auto tbl1 = TableApplySolver::isHit(mmbr, refMap, typeMap);
            tbl_apply = tbl != nullptr || tbl1 != nullptr;
        }
        // Simplified method call, with arguments substituted
        auto simplified = new IR::MethodCallExpression(
            mce->srcInfo, method, mce->typeArguments, args);
        typeMap->setType(simplified, type);
        result->final = simplified;
        // See whether we assign the result of the call to a temporary
        if (!type->is<IR::Type_Void>() &&  // no return type
            !tbl_apply &&                  // not a table.apply call
            !resultNotUsed) {              // result of call is not used
            auto tmp = refMap->newName("tmp");
            auto decl = new IR::Declaration_Variable(IR::ID(tmp, nullptr), type);
            result->temporaries->push_back(decl);
            auto left = new IR::PathExpression(IR::ID(tmp, nullptr));
            auto stat = new IR::AssignmentStatement(left, simplified);
            result->statements->push_back(stat);
            result->final = left->clone();
            typeMap->setType(result->final, type);
            LOG3(mce << " replaced with " << left << " = " << simplified);
        } else {
            if (tbl_apply) {
                result->final = simplified;
            } else {
                result->statements->push_back(
                    new IR::MethodCallStatement(mce->srcInfo, simplified));
                result->final = nullptr;
            }
        }
        result->statements->append(*copyBack);
        prune();
        return result->final;
    }

 public:
    DismantleExpression(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), leftValue(false) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        result = new EvaluationOrder(refMap);
        setName("DismantleExpressions");
    }
    EvaluationOrder* dismantle(const IR::Expression* expression,
                               bool isLeftValue, bool resultNotUsed = false) {
        LOG3("Dismantling " << dbp(expression) << (isLeftValue ? " on left" : " on right"));
        leftValue = isLeftValue;
        this->resultNotUsed = resultNotUsed;
        (void)expression->apply(*this);
        LOG3("Result is " << result->final);
        return result;
    }
};
}  // namespace

const IR::Node* DoSimplifyExpressions::postorder(IR::Function* function) {
    if (toInsert.empty())
        return function;
    auto body = new IR::BlockStatement(function->body->srcInfo);
    for (auto a : toInsert)
        body->push_back(a);
    for (auto s : function->body->components)
        body->push_back(s);
    function->body = body;
    toInsert.clear();
    return function;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Parser* parser) {
    if (toInsert.empty())
        return parser;
    parser->parserLocals.append(toInsert);
    toInsert.clear();
    return parser;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Control* control) {
    if (toInsert.empty())
        return control;
    control->controlLocals.append(toInsert);
    toInsert.clear();
    return control;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Action* action) {
    if (toInsert.empty())
        return action;
    auto body = new IR::BlockStatement(action->body->srcInfo);
    for (auto a : toInsert)
        body->push_back(a);
    for (auto s : action->body->components)
        body->push_back(s);
    action->body = body;
    toInsert.clear();
    return action;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::ParserState* state) {
    if (state->selectExpression == nullptr)
        return state;
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(state->selectExpression, false);
    CHECK_NULL(parts);
    if (parts->simple())
        return state;
    toInsert.append(*parts->temporaries);
    state->components.append(*parts->statements);
    state->selectExpression = parts->final;
    return state;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::AssignmentStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto left = dm.dismantle(statement->left, true)->final;
    CHECK_NULL(left);
    auto parts = dm.dismantle(statement->right, false);
    CHECK_NULL(parts);
    toInsert.append(*parts->temporaries);
    auto right = parts->final;
    CHECK_NULL(right);
    parts->statements->push_back(new IR::AssignmentStatement(statement->srcInfo, left, right));
    auto block = new IR::BlockStatement(*parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::MethodCallStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->methodCall, false, true);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto block = new IR::BlockStatement(*parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::ReturnStatement* statement) {
    if (statement->expression == nullptr)
        return statement;
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->expression, false);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(new IR::ReturnStatement(statement->srcInfo, expr));
    auto block = new IR::BlockStatement(*parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::IfStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->condition, false);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(new IR::IfStatement(statement->srcInfo, expr,
                                                     statement->ifTrue, statement->ifFalse));
    auto block = new IR::BlockStatement(*parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::SwitchStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->expression, false);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(
        new IR::SwitchStatement(statement->srcInfo, expr, std::move(statement->cases)));
    auto block = new IR::BlockStatement(*parts->statements);
    return block;
}

}  // namespace P4
