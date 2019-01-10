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

#include "simplifyDefUse.h"
#include "frontends/p4/def_use.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/sideEffects.h"

namespace P4 {

namespace {

class HasUses {
    // Set of program points whose left-hand sides are used elsewhere
    // in the program together with their use count
    std::unordered_map<const IR::Node*, unsigned> used;

 public:
    HasUses() = default;
    void add(const ProgramPoints* points) {
        for (auto e : *points) {
            auto last = e.last();
            if (last != nullptr) {
                LOG3("Found use for " << dbp(last) << " " <<
                     (last->is<IR::Statement>() ? last : nullptr));
                auto it = used.find(last);
                if (it == used.end())
                    used.emplace(last, 1);
                else
                    used.emplace(last, it->second + 1);
            }
        }
    }
    void remove(const ProgramPoint point) {
        auto last = point.last();
        auto it = used.find(last);
        if (it->second == 1)
            used.erase(it);
        else
            used.emplace(last, it->second - 1);
    }
    bool hasUses(const IR::Node* node) const
    { return used.find(node) != used.end(); }
};

// Run for each parser and control separately
// Somewhat of a misnamed pass -- the main point of this pass is to find all the uses
// of each definition, and fill in the `hasUses` output with all the definitions that have
// uses so RemoveUnused can remove unused things.  It incidentally finds uses that have
// no definitions and issues uninitialized warnings about them.
class FindUninitialized : public Inspector {
    ProgramPoint    context;    // context as of the last call or state transition
    ReferenceMap*   refMap;
    TypeMap*        typeMap;
    AllDefinitions* definitions;
    bool            lhs;  // checking the lhs of an assignment
    ProgramPoint    currentPoint;  // context of the current expression/statement
    // For some simple expresssions keep here the read location sets.
    // This does not include location sets read by subexpressions.
    std::map<const IR::Expression*, const LocationSet*> readLocations;
    HasUses*        hasUses;  // output

    const LocationSet* getReads(const IR::Expression* expression, bool nonNull = false) const {
        auto result = ::get(readLocations, expression);
        if (nonNull)
            BUG_CHECK(result != nullptr, "no locations known for %1%", dbp(expression));
        return result;
    }
    // 'expression' is reading the 'loc' location set
    void reads(const IR::Expression* expression, const LocationSet* loc) {
        LOG3(expression << " reads " << loc);
        CHECK_NULL(expression);
        CHECK_NULL(loc);
        readLocations.emplace(expression, loc);
    }
    bool setCurrent(const IR::Statement* statement) {
        currentPoint = ProgramPoint(context, statement);
        return false;
    }

    FindUninitialized(FindUninitialized* parent, ProgramPoint context) :
            context(context), refMap(parent->definitions->storageMap->refMap),
            typeMap(parent->definitions->storageMap->typeMap),
            definitions(parent->definitions), lhs(false), currentPoint(context),
            hasUses(parent->hasUses) { visitDagOnce = false; }

 public:
    FindUninitialized(AllDefinitions* definitions, HasUses* hasUses) :
            refMap(definitions->storageMap->refMap),
            typeMap(definitions->storageMap->typeMap),
            definitions(definitions), lhs(false), currentPoint(),
            hasUses(hasUses) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(definitions);
        CHECK_NULL(hasUses);
        visitDagOnce = false; }

    // we control the traversal order manually, so we always 'prune()'
    // (return false from preorder)

    bool preorder(const IR::ParserState* state) override {
        LOG3("FU Visiting state " << state->name);
        context = ProgramPoint(state);
        currentPoint = ProgramPoint(state);  // point before the first statement
        visit(state->components, "components");
        if (state->selectExpression != nullptr)
            visit(state->selectExpression);
        context = ProgramPoint();
        return false;
    }

    Definitions* getCurrentDefinitions() const {
        LOG3("FU Current point is (after) " << currentPoint);
        auto defs = definitions->getDefinitions(currentPoint, true);
        return defs;
    }

    void checkOutParameters(const IR::IDeclaration* block,
                            const IR::ParameterList* parameters,
                            Definitions* defs, bool checkReturn = false) {
        for (auto p : parameters->parameters) {
            if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut) {
                auto storage = definitions->storageMap->getStorage(p);
                if (storage == nullptr)
                    continue;

                const LocationSet* loc = new LocationSet(storage);
                auto points = defs->getPoints(loc);
                hasUses->add(points);
                // Check uninitialized non-headers (headers can be invalid).
                // inout parameters can never match here, so we could skip them.
                loc = storage->removeHeaders();
                points = defs->getPoints(loc);
                if (points->containsBeforeStart())
                    DIAGNOSE_WARN("uninitialized_out_param",
                                  "out parameter %1% may be uninitialized when "
                                  "%2% terminates", p, block->getName());
            }
        }

        if (checkReturn) {
            // check returned value
            auto storage = definitions->storageMap->getRetVal();
            if (storage != nullptr && defs->hasLocation(storage))
                // If this definition is "live" it means that we have
                // not returned on all paths; returns kill this definition.
                ::error("Function %1% does not return a value on all paths", block);
        }
    }

    bool preorder(const IR::P4Control* control) override {
        LOG3("FU Visiting control " << control->name << "[" << control->id << "]");
        BUG_CHECK(context.isBeforeStart(), "non-empty context in FindUnitialized::P4Control");
        currentPoint = ProgramPoint(control);
        for (auto d : control->controlLocals)
            if (d->is<IR::Declaration_Instance>())
                // visit virtual Function implementation if any
                visit(d);
        visit(control->body);
        checkOutParameters(
            control, control->getApplyMethodType()->parameters, getCurrentDefinitions());
        return false;
    }

    bool preorder(const IR::Function* func) override {
        LOG3("FU Visiting function " << func->name << "[" << func->id << "]");
        LOG5(func);
        // FIXME -- this throws away the context of the current point, which seems wrong,
        // FIXME -- but otherwise analysis fails
        currentPoint = ProgramPoint(func);
        visit(func->body);
        bool checkReturn = !func->type->returnType->is<IR::Type_Void>();
        checkOutParameters(func, func->type->parameters, getCurrentDefinitions(), checkReturn);
        return false;
    }

    bool preorder(const IR::P4Parser* parser) override {
        LOG3("FU Visiting parser " << parser->name << "[" << parser->id << "]");
        visit(parser->states, "states");
        auto accept = ProgramPoint(parser->getDeclByName(IR::ParserState::accept)->getNode());
        auto acceptdefs = definitions->getDefinitions(accept, true);
        if (!acceptdefs->empty())
            // acceptdefs is empty when the accept state is unreachable
            checkOutParameters(parser, parser->getApplyMethodType()->parameters, acceptdefs);
        return false;
    }

    bool preorder(const IR::AssignmentStatement* statement) override {
        LOG3("FU Visiting " << dbp(statement) << " " << statement);
        auto assign = statement->to<IR::AssignmentStatement>();
        lhs = true;
        visit(assign->left);
        lhs = false;
        visit(assign->right);
        return setCurrent(statement);
    }

    bool preorder(const IR::ReturnStatement* statement) override {
        LOG3("FU Visiting " << statement);
        if (statement->expression != nullptr)
            visit(statement->expression);
        return setCurrent(statement);
    }

    bool preorder(const IR::MethodCallStatement* statement) override {
        LOG3("FU Visiting " << statement);
        visit(statement->methodCall);
        return setCurrent(statement);
    }

    bool preorder(const IR::BlockStatement* statement) override {
        LOG3("FU Visiting " << statement);
        visit(statement->components, "components");
        return setCurrent(statement);
    }

    bool preorder(const IR::IfStatement* statement) override {
        LOG3("FU Visiting " << statement);
        visit(statement->condition);
        auto saveCurrent = currentPoint;
        visit(statement->ifTrue);
        if (statement->ifFalse != nullptr) {
            currentPoint = saveCurrent;
            visit(statement->ifFalse);
        }
        return setCurrent(statement);
    }

    bool preorder(const IR::SwitchStatement* statement) override {
        LOG3("FU Visiting " << statement);
        visit(statement->expression);
        currentPoint = ProgramPoint(context, statement->expression);  // CTD -- added context
        auto saveCurrent = currentPoint;
        for (auto c : statement->cases) {
            if (c->statement != nullptr) {
                LOG3("Visiting " << c);
                currentPoint = saveCurrent;
                visit(c);
            }
        }
        return setCurrent(statement);
    }

    ////////////////// Expressions

    bool preorder(const IR::Literal* expression) override {
        reads(expression, LocationSet::empty);
        return false;
    }

    bool preorder(const IR::TypeNameExpression* expression) override {
        reads(expression, LocationSet::empty);
        return false;
    }

    // Check whether the expression the child of a Member or
    // ArrayIndex.  I.e., for and expression such as a.x within a
    // larger expression a.x.b it returns "false".  This is because
    // the expression is not reading a.x, it is reading just a.x.b.
    // ctx must be the context of the current expression in the
    // visitor.
    bool isFinalRead(const Visitor::Context* ctx, const IR::Expression* expression) {
        if (ctx == nullptr)
            return true;

        // If this expression is a child of a Member of a left
        // child of an ArrayIndex then we don't report it here, only
        // in the parent.
        auto parentexp = ctx->node->to<IR::Expression>();
        if (parentexp != nullptr) {
            if (parentexp->is<IR::Member>())
                return false;
            if (parentexp->is<IR::ArrayIndex>()) {
                // Since we are doing the visit using a custom order,
                // ctx->child_index is not accurate, so we check
                // manually whether this is the left child.
                auto ai = parentexp->to<IR::ArrayIndex>();
                if (ai->left == expression)
                    return false;
            }
        }
        return true;
    }

    // Keeps track of which expression producers have uses in the given expression
    void registerUses(const IR::Expression* expression, bool reportUninitialized = true) {
        if (!isFinalRead(getContext(), expression))
            return;
        const LocationSet* read = getReads(expression);
        if (read == nullptr || read->isEmpty())
            return;
        auto currentDefinitions = getCurrentDefinitions();
        auto points = currentDefinitions->getPoints(read);
        if (reportUninitialized && !lhs && points->containsBeforeStart()) {
            // Do not report uninitialized values on the LHS.
            // This could happen if we are writing to an array element
            // with an unknown index.
            auto type = typeMap->getType(expression, true);
            cstring message;
            if (type->is<IR::Type_Base>())
                message = "%1% may be uninitialized";
            else
                message = "%1% may not be completely initialized";
            DIAGNOSE_WARN("uninitialized_use", message, expression);
        }
        hasUses->add(points);
    }

    // For the following we compute the read set and save it.
    // We check the read set later.
    bool preorder(const IR::PathExpression* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        if (lhs) {
            reads(expression, LocationSet::empty);
            return false;
        }
        auto decl = refMap->getDeclaration(expression->path, true);
        auto storage = definitions->storageMap->getStorage(decl);
        const LocationSet* result;
        if (storage != nullptr)
            result = new LocationSet(storage);
        else
            result = LocationSet::empty;
        reads(expression, result);
        registerUses(expression);
        return false;
    }

    bool preorder(const IR::P4Action* action) override {
        LOG3("FU Visiting " << action);
        currentPoint = ProgramPoint(context, action);
        visit(action->body);
        checkOutParameters(action, action->parameters, getCurrentDefinitions());
        return false;
    }

    bool preorder(const IR::P4Table* table) override {
        LOG3("FU Visiting " << table->name);
        auto savePoint = ProgramPoint(context, table);
        currentPoint = savePoint;
        auto key = table->getKey();
        visit(key);
        auto actions = table->getActionList();
        for (auto ale : actions->actionList) {
            BUG_CHECK(ale->expression->is<IR::MethodCallExpression>(),
                      "%1%: unexpected entry in action list", ale);
            visit(ale->expression);
            currentPoint = savePoint;  // restore the current point
                                    // it is modified by the inter-procedural analysis
        }
        return false;
    }

    bool preorder(const IR::MethodCallExpression* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        visit(expression->method);
        auto mi = MethodInstance::resolve(expression, refMap, typeMap);
        if (auto bim = mi->to<BuiltInMethod>()) {
            auto base = getReads(bim->appliedTo, true);
            cstring name = bim->name.name;
            if (name == IR::Type_Stack::push_front ||
                name == IR::Type_Stack::pop_front) {
                // Reads all array fields
                reads(expression, base);
                registerUses(expression, false);
                return false;
            } else if (name == IR::Type_Header::isValid) {
                auto storage = base->getField(StorageFactory::validFieldName);
                reads(expression, storage);
                registerUses(expression);
                return false;
            }
        }

        // Symbolically call some methods (actions and tables, extern methods)
        std::vector <const IR::IDeclaration *> callee;
        if (auto ac = mi->to<ActionCall>()) {
            callee.push_back(ac->action);
        } else if (mi->isApply()) {
            auto am = mi->to<ApplyMethod>();
            if (am->isTableApply()) {
                auto table = am->object->to<IR::P4Table>();
                callee.push_back(table);
            }
            // skip control apply calls...
        } else if (auto em = mi->to<ExternMethod>()) {
            LOG4("##call to extern " << expression);
            callee = em->mayCall(); }
        if (!callee.empty()) {
            LOG3("Analyzing " << callee);
            ProgramPoint pt(context, expression);
            FindUninitialized fu(this, pt);
            for (auto c : callee)
                (void)c->getNode()->apply(fu);
        }

        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            auto expr = mi->substitution.lookup(p);
            if (p->direction == IR::Direction::Out) {
                // out parameters are not read; they behave as if they are on
                // the LHS of an assignment
                bool save = lhs;
                lhs = true;
                visit(expr);
                lhs = save;
            } else {
                visit(expr);
            }
        }
        reads(expression, LocationSet::empty);
        return false;
    }

    bool preorder(const IR::Member* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        visit(expression->expr);
        if (expression->expr->is<IR::TypeNameExpression>()) {
            // this is a constant
            reads(expression, LocationSet::empty);
            return false;
        }
        if (TableApplySolver::isHit(expression, refMap, typeMap) ||
            TableApplySolver::isActionRun(expression, refMap, typeMap))
            return false;
        auto type = typeMap->getType(expression, true);
        if (type->is<IR::Type_Method>())
            // dealt within the parent
            return false;

        auto storage = getReads(expression->expr, true);
        auto basetype = typeMap->getType(expression->expr, true);
        if (basetype->is<IR::Type_Stack>()) {
            if (expression->member.name == IR::Type_Stack::next ||
                expression->member.name == IR::Type_Stack::last) {
                reads(expression, storage);
                registerUses(expression, false);
                if (!lhs && expression->member.name == IR::Type_Stack::next)
                    ::warning(ErrorType::WARN_UNINITIALIZED,
                              "%1%: reading uninitialized value", expression);
                return false;
            } else if (expression->member.name == IR::Type_Stack::lastIndex) {
                auto index = storage->getArrayLastIndex();
                reads(expression, index);
                registerUses(expression, false);
                return false;
            }
        }

        auto fields = storage->getField(expression->member);
        reads(expression, fields);
        registerUses(expression);
        return false;
    }

    bool preorder(const IR::Slice* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        bool save = lhs;
        lhs = false;  // slices on the LHS also read the data
        visit(expression->e0);
        auto storage = getReads(expression->e0, true);
        reads(expression, storage);   // true even in LHS
        registerUses(expression);
        lhs = save;
        return false;
    }

    bool preorder(const IR::ArrayIndex* expression) override {
        LOG3("FU Visiting [" << expression->id << "]: " << expression);
        if (expression->right->is<IR::Constant>()) {
            if (lhs) {
                reads(expression, LocationSet::empty);
            } else {
                auto cst = expression->right->to<IR::Constant>();
                auto index = cst->asInt();
                visit(expression->left);
                auto storage = getReads(expression->left, true);
                auto result = storage->getIndex(index);
                reads(expression, result);
            }
        } else {
            // We model a write with an unknown index as a read/write
            // to the whole array.
            auto save = lhs;
            lhs = false;
            visit(expression->right);
            visit(expression->left);
            auto storage = getReads(expression->left, true);
            lhs = save;
            reads(expression, storage);
        }
        registerUses(expression);
        return false;
    }

    bool preorder(const IR::Operation_Unary* expression) override {
        BUG_CHECK(!lhs, "%1%: Unary operation on LHS?", expression);
        visit(expression->expr);
        // This expression in fact reads the result of the operation,
        // which is a temporary storage location, which we do not model
        // in the def-use analysis.
        reads(expression, LocationSet::empty);
        registerUses(expression);
        return false;
    }

    bool preorder(const IR::Operation_Binary* expression) override {
        BUG_CHECK(!lhs, "%1%: Binary operation on LHS?", expression);
        visit(expression->left);
        visit(expression->right);
        // This expression in fact reads the result of the operation,
        // which is a temporary storage location, which we do not model
        // in the def-use analysis.
        reads(expression, LocationSet::empty);
        registerUses(expression);
        return false;
    }
};

class RemoveUnused : public Transform {
    // TODO: remove transitively unused
    const HasUses* hasUses;

 public:
    explicit RemoveUnused(const HasUses* hasUses) : hasUses(hasUses)
    { CHECK_NULL(hasUses); setName("RemoveUnused"); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override {
        if (!hasUses->hasUses(getOriginal())) {
            LOG3("Removing statement " << getOriginal() << " " << statement);
            SideEffects se(nullptr, nullptr);
            (void)statement->right->apply(se);

            if (se.nodeWithSideEffect != nullptr) {
                // We expect that at this point there can't be more than 1
                // method call expression in each statement.
                BUG_CHECK(se.sideEffectCount == 1, "%1%: too many side-effect in one expression",
                          statement->right);
                BUG_CHECK(se.nodeWithSideEffect->is<IR::MethodCallExpression>(),
                          "%1%: expected method call", se.nodeWithSideEffect);
                auto mce = se.nodeWithSideEffect->to<IR::MethodCallExpression>();
                return new IR::MethodCallStatement(statement->srcInfo, mce);
            }
            return new IR::EmptyStatement();
        }
        return statement;
    }
};

// Run for each parser and control separately.
class ProcessDefUse : public PassManager {
    AllDefinitions *definitions;
    HasUses         hasUses;
 public:
    ProcessDefUse(ReferenceMap* refMap, TypeMap* typeMap) :
            definitions(new AllDefinitions(refMap, typeMap)) {
        passes.push_back(new ComputeWriteSet(definitions));
        passes.push_back(new FindUninitialized(definitions, &hasUses));
        passes.push_back(new RemoveUnused(&hasUses));
        setName("ProcessDefUse");
    }
};
}  // namespace

const IR::Node* DoSimplifyDefUse::process(const IR::Node* node) {
    ProcessDefUse process(refMap, typeMap);
    LOG5("ProcessDefUse of:\n" << node);
    return node->apply(process);
}

}  // namespace P4
