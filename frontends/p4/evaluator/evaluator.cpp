#include "blockMap.h"
#include "evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/constantFolding.h"

namespace P4 {

Visitor::profile_t Evaluator::init_apply(const IR::Node* node) {
    BUG_CHECK(node->is<IR::P4Program>(),
              "Evaluation should be invoked on a program, not a %1%", node);
    return Inspector::init_apply(node);
}

IR::Block* Evaluator::currentBlock() const {
    BUG_CHECK(!blockStack.empty(), "Empty stack");
    return blockStack.back();
}

void Evaluator::setValue(const IR::Node* node, const IR::CompileTimeValue* constant) {
    CHECK_NULL(node);
    CHECK_NULL(constant);
    auto block = currentBlock();
    LOG2("Value of " << node << " is " << constant << " in block " << block->id);
    block->setValue(node, constant);
}

const IR::CompileTimeValue* Evaluator::getValue(const IR::Node* node) const {
    CHECK_NULL(node);
    auto block = currentBlock();
    return block->getValue(node);
}

////////////////////////////// visitor methods ////////////////////////////////////

bool Evaluator::preorder(const IR::P4Program* program) {
    LOG1("Evaluating " << program);
    blockMap->setProgram(program);
    auto toplevel = new IR::ToplevelBlock(program->srcInfo, program);
    blockMap->toplevelBlock = toplevel;

    blockStack.push_back(toplevel);
    for (auto d : *program->declarations) {
        if (d->is<IR::Type_Declaration>())
            // we will visit various containers and externs only when we instantiated them
            continue;
        visit(d);
    }
    blockStack.pop_back();
    LOG1("BlockMap:" << std::endl << blockMap);
    return false;
}

bool Evaluator::preorder(const IR::Declaration_Constant* decl) {
    LOG1("Evaluating " << decl);
    visit(decl->initializer);
    auto value = getValue(decl);
    if (value != nullptr)
        setValue(decl, value);
    return false;
}

std::vector<const IR::CompileTimeValue*>*
Evaluator::evaluateArguments(const IR::Vector<IR::Expression>* arguments) {
    P4::ConstantFolding cf(blockMap->refMap, nullptr);
    auto values = new std::vector<const IR::CompileTimeValue*>();
    for (auto e : *arguments) {
        auto folded = e->apply(cf);  // constant fold argument
        CHECK_NULL(folded);
        visit(folded);  // recursive evaluation
        auto value = getValue(folded);
        if (value == nullptr) {
            ::error("%1%: Cannot evaluate to a compile-time constant", e);
            return nullptr;
        } else {
            values->push_back(value);
        }
    }
    return values;
}

const IR::Block*
Evaluator::processConstructor(const IR::Node* node, const IR::Type* type,
                              const IR::Type* instanceType,  // type may be specialized
                              const IR::Vector<IR::Expression>* arguments) {
    LOG1("Evaluating constructor " << type);
    if (type->is<IR::Type_SpecializedCanonical>())
        type = type->to<IR::Type_SpecializedCanonical>()->substituted;

    if (type->is<IR::Type_Extern>()) {
        auto exttype = type->to<IR::Type_Extern>();
        auto constructor = exttype->lookupMethod(exttype->name.name, arguments->size());
        BUG_CHECK(constructor != nullptr,
                  "Type %1% has no constructor with %2% arguments",
                  exttype, arguments->size());
        auto block = new IR::ExternBlock(node->srcInfo, node, instanceType, exttype, constructor);
        blockStack.push_back(block);
        auto values = evaluateArguments(arguments);
        if (values == nullptr)
            return nullptr;
        block->instantiate(values);
        blockStack.pop_back();
        return block;
    } else if (type->is<IR::P4Control>()) {
        auto cont = blockMap->typeMap->getContainerFromType(type)->to<IR::P4Control>();
        BUG_CHECK(cont != nullptr, "Could not locate container for %1%", type);
        auto block = new IR::ControlBlock(node->srcInfo, node, instanceType, cont);
        auto values = evaluateArguments(arguments);
        if (values == nullptr)
            return nullptr;
        block->instantiate(values);
        blockStack.push_back(block);
        for (auto a : *cont->statefulEnumerator())
            visit(a);
        blockStack.pop_back();
        return block;
    } else if (type->is<IR::P4Parser>()) {
        auto cont = blockMap->typeMap->getContainerFromType(type)->to<IR::P4Parser>();
        BUG_CHECK(cont != nullptr, "Could not locate container for %1%", type);
        auto block = new IR::ParserBlock(node->srcInfo, node, instanceType, cont);
        blockStack.push_back(block);
        auto values = evaluateArguments(arguments);
        if (values == nullptr)
            return nullptr;
        block->instantiate(values);
        for (auto a : *cont->stateful)
            visit(a);
        for (auto a : *cont->states)
            visit(a);
        blockStack.pop_back();
        return block;
    } else if (type->is<IR::Type_Package>()) {
        auto block = new IR::PackageBlock(node->srcInfo, node, instanceType,
                                          type->to<IR::Type_Package>());
        blockStack.push_back(block);
        auto values = evaluateArguments(arguments);
        if (values == nullptr)
            return nullptr;
        block->instantiate(values);
        blockStack.pop_back();
        return block;
    }

    BUG("Unhandled case %1%: type is %2%", node, type);
    return nullptr;
}

bool Evaluator::preorder(const IR::Member* expression) {
    LOG1("Evaluating " << expression);
    auto type = blockMap->typeMap->getType(expression->expr, true);
    const IR::IDeclaration* decl = nullptr;
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
    LOG1("Evaluating " << expression);
    auto decl = blockMap->refMap->getDeclaration(expression->path, true);
    auto val = getValue(decl->getNode());
    if (val != nullptr)
        setValue(expression, val);
    return false;
}

bool Evaluator::preorder(const IR::Declaration_Instance* inst) {
    LOG1("Evaluating " << inst);
    auto type = blockMap->typeMap->getType(inst->type, true);
    auto instanceType = blockMap->typeMap->getType(inst, true);
    auto block = processConstructor(inst, type, instanceType, inst->arguments);
    setValue(inst, block);
    return false;
}

bool Evaluator::preorder(const IR::ConstructorCallExpression* expr) {
    LOG1("Evaluating " << expr);
    auto type = blockMap->typeMap->getType(expr->type, true);
    auto instanceType = blockMap->typeMap->getType(expr, true);
    auto block = processConstructor(expr, type, instanceType, expr->arguments);
    setValue(expr, block);
    return false;
}

bool Evaluator::preorder(const IR::P4Table* table) {
    LOG1("Evaluating " << table);
    auto block = new IR::TableBlock(table->srcInfo, table, table);
    setValue(table, block);
    blockStack.push_back(block);
    visit(table->properties);
    blockStack.pop_back();
    return false;
}

bool Evaluator::preorder(const IR::TableProperty* prop) {
    LOG1("Evaluating " << prop);
    visit(prop->value);
    auto value = getValue(prop->value);
    if (value != nullptr)
        setValue(prop, value);
    return false;
}

//////////////////////////////////////

EvaluatorPass::EvaluatorPass(bool anyOrder) :
        refMap(new ReferenceMap), typeMap(new TypeMap()), blockMap(new BlockMap(refMap, typeMap)) {
    passes.emplace_back(new P4::ResolveReferences(refMap, anyOrder));
    passes.emplace_back(new P4::TypeChecker(refMap, typeMap));
    passes.emplace_back(new P4::Evaluator(blockMap));
}

}  // namespace P4
