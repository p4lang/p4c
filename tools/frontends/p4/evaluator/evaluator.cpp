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

#include "evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/constantFolding.h"

namespace P4 {

Visitor::profile_t Evaluator::init_apply(const IR::Node* node) {
    BUG_CHECK(node->is<IR::P4Program>(),
              "Evaluation should be invoked on a program, not a %1%", node);
    // In fact, some passes are intentionally run with stale maps...
    // refMap->validateMap(node);
    // typeMap->validateMap(node);
    return Inspector::init_apply(node);
}

IR::Block* Evaluator::currentBlock() const {
    BUG_CHECK(!blockStack.empty(), "Empty stack");
    return blockStack.back();
}

void Evaluator::pushBlock(IR::Block* block) {
    LOG2("Set current block to " << dbp(block));
    blockStack.push_back(block);
}

void Evaluator::popBlock(IR::Block* block) {
    LOG2("Remove current block " << dbp(block));
    BUG_CHECK(!blockStack.empty(), "Empty stack");
    BUG_CHECK(blockStack.back() == block, "%1%: incorrect block popped from stack: %2%",
              block, blockStack.back());
    blockStack.pop_back();
}

void Evaluator::setValue(const IR::Node* node, const IR::CompileTimeValue* constant) {
    CHECK_NULL(node);
    auto block = currentBlock();
    LOG3("Set " << dbp(node) << " to " << dbp(constant) << " in " << dbp(block));
    block->setValue(node, constant);
}

bool Evaluator::hasValue(const IR::Node* node) const {
    CHECK_NULL(node);
    auto block = currentBlock();
    return block->hasValue(node) || toplevelBlock->hasValue(node);
}

const IR::CompileTimeValue* Evaluator::getValue(const IR::Node* node) const {
    CHECK_NULL(node);
    auto block = currentBlock();
    const IR::CompileTimeValue* result = nullptr;

    if (block->hasValue(node)) {
        result = block->getValue(node);
        LOG3("Looked up " << dbp(node) << " in " << dbp(block) << " got " << dbp(result));
    } else if (block != toplevelBlock) {
        // Try to lookup this value in the toplevel block:
        // this is needed for global declarations.
        if (toplevelBlock->hasValue(node))
            result = toplevelBlock->getValue(node);
    }
    return result;
}

////////////////////////////// visitor methods ////////////////////////////////////

bool Evaluator::preorder(const IR::P4Program* program) {
    LOG2("Evaluating " << dbp(program));
    toplevelBlock = new IR::ToplevelBlock(program->srcInfo, program);

    pushBlock(toplevelBlock);
    for (auto d : program->objects) {
        if (d->is<IR::Type_Declaration>())
            // we will visit various containers and externs only when we instantiated them
            continue;
        visit(d);
    }
    popBlock(toplevelBlock);
    std::stringstream str;
    toplevelBlock->dbprint_recursive(str);
    LOG2(str.str());
    return false;
}

bool Evaluator::preorder(const IR::Declaration_Constant* decl) {
    LOG2("Evaluating " << dbp(decl));
    visit(decl->initializer);
    auto value = getValue(decl);
    setValue(decl, value);
    return false;
}

std::vector<const IR::CompileTimeValue*>*
Evaluator::evaluateArguments(
    const IR::ParameterList* parameters,
    const IR::Vector<IR::Argument>* arguments, IR::Block* context) {
    LOG2("Evaluating arguments in " << dbp(context));
    P4::DoConstantFolding cf(refMap, nullptr);
    auto values = new std::vector<const IR::CompileTimeValue*>();
    pushBlock(context);

    ParameterSubstitution substitution;
    substitution.populate(parameters, arguments);

    for (auto p : parameters->parameters) {
        auto arg = substitution.lookup(p);
        if (arg == nullptr) {
            BUG_CHECK(p->isOptional(), "Missing parameter %1%", p);
            values->push_back(nullptr);
            continue;
        }

        auto folded = arg->expression->apply(cf);  // constant fold argument
        CHECK_NULL(folded);
        visit(folded);  // recursive evaluation
        if (!hasValue(folded)) {
            ::error("%1%: Cannot evaluate to a compile-time constant", arg->expression);
            popBlock(context);
            return nullptr;
        } else {
            auto value = getValue(folded);
            values->push_back(value);
        }
    }
    popBlock(context);
    return values;
}

const IR::Block*
Evaluator::processConstructor(
    const IR::Node* node,  // Node that invokes constructor:
                           // could be a Declaration_Instance or a ConstructorCallExpression.
    const IR::Type* type,  // Type that appears in the program that is instantiated.
    const IR::Type* instanceType,  // Actual canonical type of generated instance.
    const IR::Vector<IR::Argument>* arguments) {  // Constructor arguments
    LOG2("Evaluating constructor " << dbp(type));
    if (type->is<IR::Type_Specialized>())
        type = type->to<IR::Type_Specialized>()->baseType;
    const IR::IDeclaration* decl;
    if (type->is<IR::Type_Name>()) {
        auto tn = type->to<IR::Type_Name>();
        decl = refMap->getDeclaration(tn->path, true);
    } else {
        BUG_CHECK(type->is<IR::IDeclaration>(), "%1%: expected a type declaration", type);
        decl = type->to<IR::IDeclaration>();
    }

    auto current = currentBlock();
    if (decl->is<IR::Type_Extern>()) {
        auto exttype = decl->to<IR::Type_Extern>();
        // We lookup the method in the instanceType, because it may contain compiler-synthesized
        // constructors with zero arguments that may not appear in the original extern declaration.
        auto canon = instanceType;
        if (canon->is<IR::Type_SpecializedCanonical>())
            canon = canon->to<IR::Type_SpecializedCanonical>()->substituted->to<IR::Type>();
        BUG_CHECK(canon->is<IR::Type_Extern>(), "%1%: expected an extern", canon);
        auto constructor = canon->to<IR::Type_Extern>()->lookupConstructor(arguments);
        BUG_CHECK(constructor != nullptr,
                  "Type %1% has no constructor with %2% arguments",
                  exttype, arguments->size());
        auto block = new IR::ExternBlock(node->srcInfo, node, instanceType, exttype, constructor);
        pushBlock(block);
        auto values = evaluateArguments(constructor->type->parameters, arguments, current);
        if (values != nullptr)
            block->instantiate(values);
        popBlock(block);
        return block;
    } else if (decl->is<IR::P4Control>()) {
        auto cont = decl->to<IR::P4Control>();
        auto block = new IR::ControlBlock(node->srcInfo, node, instanceType, cont);
        pushBlock(block);
        auto values = evaluateArguments(cont->getConstructorParameters(), arguments, current);
        if (values != nullptr) {
            block->instantiate(values);
            for (auto a : cont->controlLocals)
                visit(a);
        }
        popBlock(block);
        return block;
    } else if (decl->is<IR::P4Parser>()) {
        auto cont = decl->to<IR::P4Parser>();
        auto block = new IR::ParserBlock(node->srcInfo, node, instanceType, cont);
        pushBlock(block);
        auto values = evaluateArguments(cont->getConstructorParameters(), arguments, current);
        if (values != nullptr) {
            block->instantiate(values);
            for (auto a : cont->parserLocals)
                visit(a);
            for (auto a : cont->states)
                visit(a);
        }
        popBlock(block);
        return block;
    } else if (decl->is<IR::Type_Package>()) {
        auto package = decl->to<IR::Type_Package>();
        auto block = new IR::PackageBlock(node->srcInfo, node, instanceType, package);
        pushBlock(block);
        auto values = evaluateArguments(package->constructorParams, arguments, current);
        if (values != nullptr)
            block->instantiate(values);
        popBlock(block);
        return block;
    }

    BUG("Unhandled case %1%: type is %2%", node, type);
    return nullptr;
}

bool Evaluator::preorder(const IR::Member* expression) {
    LOG2("Evaluating " << dbp(expression));
    auto type = typeMap->getType(expression->expr, true);
    const IR::IDeclaration* decl = nullptr;
    if (type->is<IR::Type_Type>())
        type = type->to<IR::Type_Type>()->type;
    if (type->is<IR::IGeneralNamespace>()) {
        auto ns = type->to<IR::IGeneralNamespace>();
        decl = ns->getDeclsByName(expression->member.name)->nextOrDefault();
    } else if (type->is<IR::ISimpleNamespace>()) {
        auto ns = type->to<IR::ISimpleNamespace>();
        decl = ns->getDeclByName(expression->member.name);
    }
    if (decl == nullptr || !decl->is<IR::Declaration_ID>())
        return false;
    setValue(expression, decl->to<IR::Declaration_ID>());
    return false;
}

bool Evaluator::preorder(const IR::PathExpression* expression) {
    LOG2("Evaluating " << dbp(expression));
    auto decl = refMap->getDeclaration(expression->path, true);
    if (hasValue(decl->getNode())) {
        auto val = getValue(decl->getNode());
        setValue(expression, val);
    }
    return false;
}

bool Evaluator::preorder(const IR::Declaration_Instance* inst) {
    LOG2("Evaluating " << dbp(inst));
    auto type = typeMap->getType(inst, true);
    auto block = processConstructor(inst, inst->type, type, inst->arguments);
    if (block != nullptr)
        setValue(inst, block);
    return false;
}

bool Evaluator::preorder(const IR::ConstructorCallExpression* expr) {
    LOG2("Evaluating " << dbp(expr));
    auto type = typeMap->getType(expr, true);
    auto block = processConstructor(expr, expr->constructedType, type, expr->arguments);
    if (block != nullptr)
        setValue(expr, block);
    return false;
}

bool Evaluator::preorder(const IR::MethodCallExpression* expr) {
    // Experimental: extern function or method call with constant arguments that
    // returns an extern instance as a factory method
    LOG2("Evaluating " << dbp(expr));
    setValue(expr, new IR::CompileTimeMethodCall(expr));
    return false;
}

bool Evaluator::preorder(const IR::P4Table* table) {
    LOG2("Evaluating " << dbp(table));
    auto block = new IR::TableBlock(table->srcInfo, table, table);
    setValue(table, block);
    pushBlock(block);
    visit(table->properties);
    popBlock(block);
    return false;
}

bool Evaluator::preorder(const IR::Property* prop) {
    LOG2("Evaluating " << dbp(prop));
    visit(prop->value);
    if (hasValue(prop->value)) {
        auto value = getValue(prop->value);
        setValue(prop, value);
    }
    return false;
}

bool Evaluator::preorder(const IR::ListExpression *list) {
    LOG2("Evaluating " << list);
    visit(list->components);
    IR::Vector<IR::Node> comp;
    for (auto e : list->components) {
        if (auto value = getValue(e)) {
            CHECK_NULL(value);
            comp.push_back(value->getNode());
        } else {
            return false;
        }
    }
    setValue(list, new IR::ListCompileTimeValue(std::move(comp)));
    return false;
}

//////////////////////////////////////

EvaluatorPass::EvaluatorPass(ReferenceMap* refMap, TypeMap* typeMap) {
    setName("EvaluatorPass");
    evaluator = new P4::Evaluator(refMap, typeMap);
    passes.emplace_back(new P4::TypeChecking(refMap, typeMap));
    passes.emplace_back(evaluator);
}

}  // namespace P4
