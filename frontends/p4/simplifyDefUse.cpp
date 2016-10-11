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

namespace P4 {

namespace {

class HasUses {
    // Set of program points whose left-hand sides are used elsewhere
    // in the program.
    std::unordered_set<const IR::Node*> used;
 public:
    HasUses() = default;
    void add(const ProgramPoints* points) {
        for (auto e : *points) {
            auto last = e.last();
            if (last != nullptr) {
                LOG1("Found use for " << last);
                used.emplace(last);
            }
        }
    }
    bool hasUses(const IR::Node* node) const
    { return used.find(node) != used.end(); }
};

// Run for each parser and control separately
class FindUninitialized : public Inspector {
    ProgramPoint    context;
    ReferenceMap*   refMap;
    TypeMap*        typeMap;
    AllDefinitions* definitions;
    bool            lhs;  // checking the lhs of an assignment
    ProgramPoint    currentPoint;
    // For some simple expresssions keep here the read location sets
    std::map<const IR::Expression*, const LocationSet*> locations;
    HasUses*        hasUses;  // output

    const LocationSet* get(const IR::Expression* expression) const {
        auto result = ::get(locations, expression);
        return result;
    }
    // 'expression' is reading the 'loc' location set
    void set(const IR::Expression* expression, const LocationSet* loc) {
        CHECK_NULL(expression);
        CHECK_NULL(loc);
        locations.emplace(expression, loc);
    }
    bool setCurrent(const IR::Statement* statement) {
        currentPoint = ProgramPoint(context, statement);
        return false;
    }

 public:
    FindUninitialized(AllDefinitions* definitions, HasUses* hasUses) :
            refMap(definitions->storageMap->refMap),
            typeMap(definitions->storageMap->typeMap),
            definitions(definitions), lhs(false), currentPoint(),
            hasUses(hasUses) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(definitions);
        CHECK_NULL(hasUses);
        setName("FindUninitialized"); }

    // we control the traversal order manually, so we always 'prune()'
    // (return false from preorder)

    bool preorder(const IR::ParserState* state) override {
        LOG1("Visiting " << dbp(state));
        context = ProgramPoint(state);
        currentPoint = ProgramPoint(state);  // point before the first statement
        visit(state->components);
        context = ProgramPoint();
        return false;
    }

    Definitions* getCurrentDefinitions() const {
        auto defs = definitions->get(currentPoint);
        CHECK_NULL(defs);
        return defs;
    }

    void checkOutParameters(const IR::IApply* block, Definitions* defs) {
        for (auto p : *block->getApplyMethodType()->parameters->parameters) {
            if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut) {
                auto storage = definitions->storageMap->getStorage(p);
                if (storage == nullptr)
                    continue;

                const LocationSet* loc = new LocationSet(storage);
                auto points = defs->get(loc);
                hasUses->add(points);
                // Check uninitialized non-headers (headers can be invalid).
                // inout parameters can never match here, so we could skip them.
                loc = storage->removeHeaders();
                points = defs->get(loc);
                if (points->containsUninitialized())
                    ::warning("out parameter %1% may be uninitialized when %2% terminates",
                              p, block);
            }
        }
    }

    bool preorder(const IR::P4Control* control) override {
        currentPoint = ProgramPoint(control);
        visit(control->body);
        checkOutParameters(control, getCurrentDefinitions());
        return false;
    }

    bool preorder(const IR::P4Parser* parser) override {
        LOG1("Visiting " << dbp(parser));
        visit(parser->states);
        auto accept = ProgramPoint(parser->getDeclByName(IR::ParserState::accept)->getNode());
        auto acceptdefs = definitions->get(accept, true);
        checkOutParameters(parser, acceptdefs);
        return false;
    }

    bool preorder(const IR::AssignmentStatement* statement) override {
        auto assign = statement->to<IR::AssignmentStatement>();
        lhs = true;
        visit(assign->left);
        lhs = false;
        visit(assign->right);
        return setCurrent(statement);
    }

    bool preorder(const IR::ReturnStatement* statement) override {
        LOG1("Visiting " << dbp(statement));
        if (statement->expression != nullptr)
            visit(statement->expression);
        return setCurrent(statement);
    }

    bool preorder(const IR::MethodCallStatement* statement) override {
        LOG1("Visiting " << dbp(statement));
        visit(statement->methodCall);
        return setCurrent(statement);
    }

    bool preorder(const IR::BlockStatement* statement) override {
        visit(statement->components);
        return setCurrent(statement);
    }

    bool preorder(const IR::IfStatement* statement) override {
        visit(statement->condition);
        visit(statement->ifTrue);
        if (statement->ifFalse != nullptr)
            visit(statement->ifFalse);
        return setCurrent(statement);
    }

    bool preorder(const IR::SwitchStatement* statement) override {
        visit(statement->expression);
        for (auto c : statement->cases)
            visit(c);
        return setCurrent(statement);
    }

    ////////////////// Expressions

    void reportUninitialized(const IR::Expression* expression) {
        auto ctx = getContext();
        const LocationSet* read = nullptr;
        if (ctx == nullptr) {
            read = get(expression);
        } else {
            auto parentexp = ctx->node->to<IR::Expression>();
            if (parentexp == nullptr ||  // e.g., Vector<Expression> for args
                (!parentexp->is<IR::Member>() &&
                 !parentexp->is<IR::ArrayIndex>())) {
                read = get(expression);
            }
        }
        if (read == nullptr)
            return;
        auto currentDefinitions = getCurrentDefinitions();
        auto points = currentDefinitions->get(read);
        if (points->containsUninitialized()) {
            auto type = typeMap->getType(expression, true);
            cstring message;
            if (type->is<IR::Type_Base>())
                message = "%1% may be uninitialized";
            else
                message = "%1% may not be completely initialized";
            ::warning(message, expression);
        }
        hasUses->add(points);
    }

    // For the following we compute the read set and save it.
    // We check the read set later.
    void postorder(const IR::PathExpression* expression) override {
        if (lhs) {
            set(expression, LocationSet::empty);
            return;
        }
        auto decl = refMap->getDeclaration(expression->path, true);
        auto storage = definitions->storageMap->getStorage(decl);
        const LocationSet* result;
        if (storage != nullptr)
            result = new LocationSet(storage);
        else
            result = LocationSet::empty;
        set(expression, result);
        reportUninitialized(expression);
    }

    bool preorder(const IR::MethodCallExpression* expression) override {
        // TODO: handle table and action invocations
        MethodCallDescription mcd(expression, refMap, typeMap);
        for (auto p : *mcd.substitution.getParameters()) {
            auto expr = mcd.substitution.lookup(p);
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
        return false;
    }

    void postorder(const IR::Member* expression) override {
        if (lhs) {
            set(expression, LocationSet::empty);
            return;
        }
        auto type = typeMap->getType(expression, true);
        if (type->is<IR::Type_Method>())
            return;
        if (expression->expr->is<IR::TypeNameExpression>())
            // this is a constant
            return;
        if (TableApplySolver::isHit(expression, refMap, typeMap) ||
            TableApplySolver::isActionRun(expression, refMap, typeMap))
            return;

        auto storage = get(expression->expr);
        CHECK_NULL(storage);

        auto basetype = typeMap->getType(expression->expr, true);
        if (basetype->is<IR::Type_Stack>()) {
            if (expression->member.name == IR::Type_Stack::next ||
                expression->member.name == IR::Type_Stack::last) {
                set(expression, storage);
                return;
            }
            if (expression->member.name == IR::Type_Stack::empty ||
                expression->member.name == IR::Type_Stack::full) {
                auto valid = storage->getValidField();
                set(expression, valid);
                return;
            }
        }

        auto fields = storage->getField(expression->member);
        set(expression, fields);
        reportUninitialized(expression);
    }

    bool preorder(const IR::ArrayIndex* expression) override {
        visit(expression->left);
        auto save = lhs;
        lhs = false;
        visit(expression->right);
        lhs = save;
        auto storage = get(expression->left);
        if (expression->right->is<IR::Constant>()) {
            if (lhs) {
                set(expression, LocationSet::empty);
            } else {
                auto cst = expression->right->to<IR::Constant>();
                auto index = cst->asInt();
                auto result = storage->getIndex(index);
                set(expression, result);
            }
        } else {
            // we model an unknown index as a read/write to the whole
            // array.
            set(expression, storage);
        }
        reportUninitialized(expression);
        return false;
    }

    void postorder(const IR::Expression* expression) override {
        reportUninitialized(expression);
    }
};

class RemoveUnused : public Transform {
    const HasUses* hasUses;
 public:
    explicit RemoveUnused(const HasUses* hasUses) : hasUses(hasUses)
    { CHECK_NULL(hasUses); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override {
        if (!hasUses->hasUses(getOriginal())) {
            LOG1("Removing statement " << statement);
            if (statement->right->is<IR::MethodCallExpression>()) {
                // keep the method for side effects
                auto mce = statement->right->to<IR::MethodCallExpression>();
                return new IR::MethodCallStatement(statement->srcInfo, mce);
            }
            return new IR::EmptyStatement();
        }
        return statement;
    }
    const IR::Node* preorder(IR::P4Action* action) override {
        // TODO
        prune();
        return action;
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
        //passes.push_back(new RemoveUnused(&hasUses));
        setName("ProcessDefUse");
    }
};
}  // namespace

const IR::Node* DoSimplifyDefUse::process(const IR::Node* node) {
    ProcessDefUse process(refMap, typeMap);
    return node->apply(process);
}

}  // namespace P4
