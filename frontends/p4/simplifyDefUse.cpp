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

namespace P4 {

namespace {

// Run for each parser and control separately
class FindUninitialized : public Inspector {
    ReferenceMap*   refMap;
    TypeMap*        typeMap;
    AllDefinitions* definitions;
    bool            lhs;  // checking the lhs of an assignment
    // For some simple expresssions keep here the read location sets
    std::map<const IR::Expression*, const LocationSet*> locations;
    Definitions*    currentDefinitions;  // set for each statement

    const LocationSet* get(const IR::Expression* expression) const {
        auto result = ::get(locations, expression);
        return result;
    }
    void set(const IR::Expression* expression, const LocationSet* loc) {
        CHECK_NULL(expression);
        CHECK_NULL(loc);
        locations.emplace(expression, loc);
    }

    void checkUninitialized(const IR::StatOrDecl* statement, ProgramPoint at) {
        if (!statement->is<IR::Statement>())
            return;
        currentDefinitions = definitions->get(at);
        if (statement->is<IR::AssignmentStatement>()) {
            auto assign = statement->to<IR::AssignmentStatement>();
            lhs = true;
            visit(assign->left);
            lhs = false;
            visit(assign->right);
        } else if (statement->is<IR::MethodCallStatement>()) {
            auto mcs = statement->to<IR::MethodCallStatement>();
            visit(mcs->methodCall);
        } else if (statement->is<IR::BlockStatement>()) {
            auto block = statement->to<IR::BlockStatement>();
            for (auto s : *block->components) {
                checkUninitialized(s, at);
                at = ProgramPoint(s);
            }
        } else {
            BUG("%1%: unexpected statement", statement);
        }
        currentDefinitions = nullptr;
    }

 public:
    FindUninitialized(ReferenceMap* refMap, TypeMap* typeMap, AllDefinitions* definitions) :
            refMap(refMap), typeMap(typeMap), definitions(definitions),
            lhs(false), currentDefinitions(nullptr) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(definitions);
        setName("FindUninitialized"); }

    bool preorder(const IR::ParserState* state) override {
        ProgramPoint cursor(state);  // point before the first statement
        for (auto s : *state->components) {
            checkUninitialized(s, cursor);  // the cursor points BEFORE the statement
            cursor = ProgramPoint(s);
        }
        return false;
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
        report(expression);
    }

    bool preorder(const IR::MethodCallExpression* expression) override {
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
        auto storage = get(expression->expr);

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
        report(expression);
    }

    bool preorder(const IR::ArrayIndex* expression) override {
        visit(expression->left);
        auto save = lhs;
        lhs = false;
        visit(expression->right);
        lhs = save;
        return true;
    }

    void postorder(const IR::ArrayIndex* expression) override {
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
        report(expression);
    }

    void report(const IR::Expression* expression) {
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
        CHECK_NULL(currentDefinitions);
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
    }

    void postorder(const IR::Expression* expression) override {
        report(expression);
    }

    bool preorder(const IR::P4Control*) override {
        // TODO: not yet implemented
        return false;
    }

    bool preorder(const IR::P4Parser* parser) override {
        visit(parser->states);
        return false;
    }
};

// Run for each parser and control separately.
class ProcessDefUse : public PassManager {
    AllDefinitions *definitions;
 public:
    ProcessDefUse(ReferenceMap* refMap, TypeMap* typeMap) :
            definitions(new AllDefinitions(refMap, typeMap)) {
        passes.push_back(new ComputeDefUse(refMap, definitions));
        passes.push_back(new FindUninitialized(refMap, typeMap, definitions));
        setName("ProcessDefUse");
    }
};
}  // namespace

const IR::Node* DoSimplifyDefUse::process(const IR::Node* node) {
    ProcessDefUse process(refMap, typeMap);
    return node->apply(process);
}

}  // namespace P4
