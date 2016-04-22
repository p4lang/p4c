#include "blockMap.h"
#include "evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/constantFolding.h"

namespace P4 {

Visitor::profile_t Evaluator::init_apply(const IR::Node* node) {
    BUG_CHECK(node->is<IR::P4Program>(),
              "Evaluation should be invoked on a program, not a %1%", node);
    // dump(node);
    return Inspector::init_apply(node);
}

IR::Block* Evaluator::currentBlock() const {
    BUG_CHECK(!blockStack.empty(), "Empty stack");
    return blockStack.back();
}

void Evaluator::pushBlock(IR::Block* block) {
    LOG1("Set current block to " << block);
    blockStack.push_back(block);
}

void Evaluator::popBlock(IR::Block* block) {
    LOG1("Remove current block " << block);
    BUG_CHECK(!blockStack.empty(), "Empty stack");
    BUG_CHECK(blockStack.back() == block, "%1%: incorrect block popped from stack: %2%",
              block, blockStack.back());
    blockStack.pop_back();
}

void Evaluator::setValue(const IR::Node* node, const IR::CompileTimeValue* constant) {
    CHECK_NULL(node);
    CHECK_NULL(constant);
    auto block = currentBlock();
    LOG2("Set " << node << " to " << constant << " in " << block);
    block->setValue(node, constant);
}

const IR::CompileTimeValue* Evaluator::getValue(const IR::Node* node) const {
    CHECK_NULL(node);
    auto block = currentBlock();
    auto result = block->getValue(node);
    LOG2("Looked up " << node << " in " << block << " got " << result);
    return result;
}

////////////////////////////// visitor methods ////////////////////////////////////

bool Evaluator::preorder(const IR::P4Program* program) {
    LOG1("Evaluating " << program);
    blockMap->setProgram(program);
    auto toplevel = new IR::ToplevelBlock(program->srcInfo, program);
    blockMap->toplevelBlock = toplevel;

    pushBlock(toplevel);
    for (auto d : *program->declarations) {
        if (d->is<IR::Type_Declaration>())
            // we will visit various containers and externs only when we instantiated them
            continue;
        visit(d);
    }
    popBlock(toplevel);
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
Evaluator::evaluateArguments(const IR::Vector<IR::Expression>* arguments, IR::Block* context) {
    P4::ConstantFolding cf(blockMap->refMap, nullptr);
    auto values = new std::vector<const IR::CompileTimeValue*>();
    pushBlock(context);
    for (auto e : *arguments) {
        auto folded = e->apply(cf);  // constant fold argument
        CHECK_NULL(folded);
        visit(folded);  // recursive evaluation
        auto value = getValue(folded);
        if (value == nullptr) {
            ::error("%1%: Cannot evaluate to a compile-time constant", e);
            popBlock(context);
            return nullptr;
        } else {
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
    const IR::Vector<IR::Expression>* arguments) {  // Constructor arguments
    LOG1("Evaluating constructor " << type);
    if (type->is<IR::Type_Specialized>())
        type = type->to<IR::Type_Specialized>()->baseType;
    const IR::IDeclaration* decl;
    if (type->is<IR::Type_Name>()) {
        auto tn = type->to<IR::Type_Name>();
        decl = blockMap->refMap->getDeclaration(tn->path, true);
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
            canon = canon->to<IR::Type_SpecializedCanonical>()->substituted;
        BUG_CHECK(canon->is<IR::Type_Extern>(), "%1%: expected an extern", canon);
        auto constructor = canon->to<IR::Type_Extern>()->lookupMethod(exttype->name.name,
                                                                      arguments->size());
        BUG_CHECK(constructor != nullptr,
                  "Type %1% has no constructor with %2% arguments",
                  exttype, arguments->size());
        auto block = new IR::ExternBlock(node->srcInfo, node, instanceType, exttype, constructor);
        pushBlock(block);
        auto values = evaluateArguments(arguments, current);
        if (values != nullptr)
            block->instantiate(values);
        popBlock(block);
        return block;
    } else if (decl->is<IR::P4Control>()) {
        auto cont = decl->to<IR::P4Control>();
        auto block = new IR::ControlBlock(node->srcInfo, node, instanceType, cont);
        pushBlock(block);
        auto values = evaluateArguments(arguments, current);
        if (values != nullptr) {
            block->instantiate(values);
            for (auto a : *cont->statefulEnumerator())
                visit(a);
        }
        popBlock(block);
        return block;
    } else if (decl->is<IR::P4Parser>()) {
        auto cont = decl->to<IR::P4Parser>();
        auto block = new IR::ParserBlock(node->srcInfo, node, instanceType, cont);
        pushBlock(block);
        auto values = evaluateArguments(arguments, current);
        if (values != nullptr) {
            block->instantiate(values);
            for (auto a : *cont->stateful)
                visit(a);
            for (auto a : *cont->states)
                visit(a);
        }
        popBlock(block);
        return block;
    } else if (decl->is<IR::Type_Package>()) {
        auto block = new IR::PackageBlock(node->srcInfo, node, instanceType,
                                          decl->to<IR::Type_Package>());
        pushBlock(block);
        auto values = evaluateArguments(arguments, current);
        if (values != nullptr)
            block->instantiate(values);
        popBlock(block);
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
    auto type = blockMap->typeMap->getType(inst, true);
    auto block = processConstructor(inst, inst->type, type, inst->arguments);
    if (block != nullptr)
        setValue(inst, block);
    return false;
}

bool Evaluator::preorder(const IR::ConstructorCallExpression* expr) {
    LOG1("Evaluating " << expr);
    auto type = blockMap->typeMap->getType(expr, true);
    auto block = processConstructor(expr, expr->type, type, expr->arguments);
    if (block != nullptr)
        setValue(expr, block);
    return false;
}

bool Evaluator::preorder(const IR::P4Table* table) {
    LOG1("Evaluating " << table);
    auto block = new IR::TableBlock(table->srcInfo, table, table);
    if (block != nullptr)
        setValue(table, block);
    pushBlock(block);
    visit(table->properties);
    popBlock(block);
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
    setName("Evaluator");
    passes.emplace_back(new P4::ResolveReferences(refMap, anyOrder));
    passes.emplace_back(new P4::TypeChecker(refMap, typeMap));
    passes.emplace_back(new P4::Evaluator(blockMap));
}

}  // namespace P4
