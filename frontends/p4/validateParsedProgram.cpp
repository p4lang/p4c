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

#include "validateParsedProgram.h"

namespace P4 {

/// Check that the type of a constant is either bit<>, int<> or int
void ValidateParsedProgram::postorder(const IR::Constant* c) {
    if (c->type != nullptr &&
        !c->type->is<IR::Type_Unknown>() &&
        !c->type->is<IR::Type_Bits>() &&
        !c->type->is<IR::Type_InfInt>())
        BUG("Invalid type %1% for constant %2%", c->type, c);
}

/// Check that underscore is not a method name
/// Check that constructors do not have a return type
/// Check that extern constructor names match the enclosing extern
void ValidateParsedProgram::postorder(const IR::Method* m) {
    if (m->name.isDontCare())
        ::error(ErrorType::ERR_INVALID, "method/function name.", m->name);
    if (auto ext = findContext<IR::Type_Extern>()) {
        if (m->name == ext->name && m->type->returnType != nullptr)
            ::error(ErrorType::ERR_INVALID, "type. Constructor cannot have a return type.", m);
        if (m->type->returnType == nullptr) {
            if (m->name != ext->name) {
                ::error(ErrorType::ERR_INVALID, "type. Method has no return type.", m);
                return;
            }
            for (auto p : *m->type->parameters)
                if (p->direction != IR::Direction::None)
                    ::error(ErrorType::ERR_INVALID,
                            "call. Constructor parameters cannot have a direction.", p);
        }
    }
}

/// Struct field names cannot be underscore
void ValidateParsedProgram::postorder(const IR::StructField* f) {
    if (f->name.isDontCare())
        ::error(ErrorType::ERR_INVALID, "field name", f->name);
}

/// Width of a bit<> or int<> type is greater than 0
void ValidateParsedProgram::postorder(const IR::Type_Bits* type) {
    if (type->expression)
        // cannot validate yet
        return;
    if (type->size <= 0)
        ::error(ErrorType::ERR_INVALID, "type size", type);
}

void ValidateParsedProgram::postorder(const IR::Type_Varbits* type) {
    if (type->expression)
        // cannot validate yet
        return;
    if (type->size <= 0)
        ::error(ErrorType::ERR_INVALID, "type size", type);
}

/// The accept and reject states cannot be implemented
void ValidateParsedProgram::postorder(const IR::ParserState* s) {
    if (s->name == IR::ParserState::accept ||
        s->name == IR::ParserState::reject)
        ::error(ErrorType::ERR_INVALID,
                "parser state.  %1% should not be implemented, it is built-in", s->name);
}

/// All parameters of a constructor must be directionless.
/// This only checks controls, parsers and packages
void ValidateParsedProgram::container(const IR::IContainer* type) {
    for (auto p : type->getConstructorParameters()->parameters)
        if (p->direction != IR::Direction::None)
            ::error(ErrorType::ERR_INVALID,
                    "aragument. Constructor parameters cannot have a direction", p);
}

/// Tables must have an 'actions' property.
void ValidateParsedProgram::postorder(const IR::P4Table* t) {
    auto ac = t->getActionList();
    if (ac == nullptr)
        ::error(ErrorType::ERR_EXPECTED,
                "`%2%' property",
                t->name, IR::TableProperties::actionsPropertyName);
}

/// Checks that the names of the three parameter lists for some constructs
/// are all distinct (type parameters, apply parameters, constructor parameters)
void ValidateParsedProgram::distinctParameters(
    const IR::TypeParameters* typeParams,
    const IR::ParameterList* apply,
    const IR::ParameterList* constr) {
    std::map<cstring, const IR::Node*> found;

    for (auto p : typeParams->parameters)
        found.emplace(p->getName(), p);
    for (auto p : apply->parameters) {
        auto it = found.find(p->getName());
        if (it != found.end())
            ::error(ErrorType::ERR_DUPLICATE, "%1% duplicates %2%.",
                    it->second, p);
        else
            found.emplace(p->getName(), p);
    }
    for (auto p : constr->parameters) {
        auto it = found.find(p->getName());
        if (it != found.end())
            ::error(ErrorType::ERR_DUPLICATE, "%1% duplicates %2%.",
                    it->second, p);
    }
}

/// Cannot invoke constructors in actions
void ValidateParsedProgram::postorder(const IR::ConstructorCallExpression* expression) {
    auto inAction = findContext<IR::P4Action>();
    if (inAction != nullptr)
        ::error(ErrorType::ERR_INVALID,
                "call. Constructor calls not allowed in actions.", expression);
}

/// Variable names cannot be underscore
void ValidateParsedProgram::postorder(const IR::Declaration_Variable* decl) {
    if (decl->name.isDontCare())
        ::error(ErrorType::ERR_INVALID, "variable name.", decl);
}

/// Instance names cannot be don't care
/// Do not declare instances in apply {} blocks, parser states or actions
void ValidateParsedProgram::postorder(const IR::Declaration_Instance* decl) {
    if (decl->name.isDontCare())
        ::error(ErrorType::ERR_INVALID, "instance name.", decl);
    if (findContext<IR::BlockStatement>() &&  // we're looking for the apply block
        findContext<IR::P4Control>() &&       // of a control
        !findContext<IR::Declaration_Instance>()) {  // but not in an instance initializer
        ::error(ErrorType::ERR_INVALID,
                "declaration. Instantiations cannot be in a control 'apply' block.", decl);
    }
    if (findContext<IR::ParserState>())
        ::error(ErrorType::ERR_INVALID,
                "declaration. Instantiations cannot be in a parser state.", decl);
    auto inAction = findContext<IR::P4Action>();
    if (inAction != nullptr)
        ::error(ErrorType::ERR_INVALID,
                "declaration. Instantiations not allowed in actions.", decl);
}

/// Constant names cannot be underscore
void ValidateParsedProgram::postorder(const IR::Declaration_Constant* decl) {
    if (decl->name.isDontCare())
        ::error(ErrorType::ERR_INVALID, "constant name.", decl);
}

/**
  * check that an entries list is declared as constant
  * The current specification supports only const entries, and we chose
  * to implement optCONST in the grammar, in part because we can provide
  * a more informative message here, and in part because it more uniform
  * handling with the rest of the properties defined for tables.
  */
void ValidateParsedProgram::postorder(const IR::EntriesList* l) {
    auto table = findContext<IR::P4Table>();
    if (table == nullptr)
        ::error(ErrorType::ERR_INVALID,
                "initializer. Table initializers must belong to a table.", l);
    auto ep = table->properties->getProperty(IR::TableProperties::entriesPropertyName);
    if (!ep->isConstant)
        ::error(ErrorType::ERR_INVALID,
                "initializer. Table initializers must be constant.", l);
}

/// Switch statements are not allowed in actions.
/// Default label in switch statement is always the last one.
void ValidateParsedProgram::postorder(const IR::SwitchStatement* statement) {
    auto inAction = findContext<IR::P4Action>();
    if (inAction != nullptr)
        ::error(ErrorType::ERR_INVALID,
                "statement. 'switch' statements not allowed in actions.", statement);
    bool defaultFound = false;
    for (auto c : statement->cases) {
        if (defaultFound) {
            ::warning(ErrorType::WARN_ORDERING, "%1%: label following default label.", c);
            break;
        }
        if (c->label->is<IR::DefaultExpression>())
            defaultFound = true;
    }
}

/// Return statements are not allowed in parsers
void ValidateParsedProgram::postorder(const IR::ReturnStatement* statement) {
    auto inParser = findContext<IR::P4Parser>();
    if (inParser != nullptr)
        ::error(ErrorType::ERR_INVALID,
                "statement. 'return' statements not allowed in parsers.", statement);
}

/// Exit statements are not allowed in parsers or functions
void ValidateParsedProgram::postorder(const IR::ExitStatement* statement) {
    auto inParser = findContext<IR::P4Parser>();
    if (inParser != nullptr)
        ::error(ErrorType::ERR_INVALID,
                "statement. 'exit' statements not allowed in parsers.", statement);
    if (findContext<IR::Function>())
        ::error(ErrorType::ERR_INVALID,
                "statement. 'exit' statements not allowed in functions.", statement);
}

void ValidateParsedProgram::postorder(const IR::P4Program* program) {
    IR::IndexedVector<IR::Node> declarations;
    for (auto decl : *program->getDeclarations()) {
        cstring name = decl->getName();
        auto existing = declarations.getDeclaration(name);
        if (existing != nullptr) {
            if (!existing->is<IR::IFunctional>() || !decl->is<IR::IFunctional>()
                || typeid(*existing) != typeid(*decl) || decl->is<IR::P4Action>()) {
                ::error(ErrorType::ERR_DUPLICATE, "%1% duplicates %2%.",
                        decl->getName(), existing->getName());
            }
        } else {
            declarations.push_back(decl->getNode());
        }
    }
}

}  // namespace P4
