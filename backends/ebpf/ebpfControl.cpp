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

#include "ebpfControl.h"
#include "ebpfType.h"
#include "ebpfTable.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"

namespace EBPF {

namespace {
class ControlBodyTranslationVisitor : public CodeGenInspector {
    const EBPFControl* control;
    std::set<const IR::Parameter*> toDereference;
    std::vector<cstring> saveAction;

 public:
    ControlBodyTranslationVisitor(const EBPFControl* control, CodeBuilder* builder) :
            CodeGenInspector(builder, control->program->typeMap), control(control) {}
    using CodeGenInspector::preorder;
    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::SwitchStatement* stat) override;
    bool preorder(const IR::IfStatement* stat) override;
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool preorder(const IR::ReturnStatement* stat) override;
    bool preorder(const IR::ExitStatement* stat) override;
    void processMethod(const P4::ExternMethod* method);
    void processApply(const P4::ApplyMethod* method);
};

bool ControlBodyTranslationVisitor::preorder(const IR::PathExpression* expression) {
    auto decl = control->program->refMap->getDeclaration(expression->path, true);
    auto param = decl->getNode()->to<IR::Parameter>();
    if (param != nullptr && toDereference.count(param) > 0)
        builder->append("*");
    builder->append(expression->path->name);  // each identifier should be unique
    return false;
}

bool ControlBodyTranslationVisitor::preorder(const IR::MethodCallExpression* expression) {
    auto mi = P4::MethodInstance::resolve(expression,
                                          control->program->refMap,
                                          control->program->typeMap);
    auto apply = mi->to<P4::ApplyMethod>();
    if (apply != nullptr) {
        processApply(apply);
        return false;
    }
    auto ext = mi->to<P4::ExternMethod>();
    if (ext != nullptr) {
        processMethod(ext);
        return false;
    }
    auto bim = mi->to<P4::BuiltInMethod>();
    if (bim != nullptr && bim->name == IR::Type_Header::isValid) {
        visit(bim->appliedTo);
        builder->append(".ebpf_valid");
        return false;
    }

    BUG("Unexpected method invocation %1%", expression);
    return false;
}

void ControlBodyTranslationVisitor::processMethod(const P4::ExternMethod* method) {
    auto block = control->controlBlock->getValue(method->object->getNode());
    BUG_CHECK(block != nullptr, "Cannot locate block for %1%", method->object);
    auto extblock = block->to<IR::ExternBlock>();
    BUG_CHECK(extblock != nullptr, "Expected external block for %1%", method->object);

    if (extblock->type->name.name != control->program->model.counterArray.name) {
        ::error("Unknown external block %1%", method->expr);
        return;
    }

    builder->blockStart();
    auto decl = extblock->node->to<IR::Declaration_Instance>();
    cstring name = nameFromAnnotation(decl->annotations, decl->name);
    auto counterMap = control->getCounter(name);
    counterMap->emitMethodInvocation(builder, method);
    builder->blockEnd(true);
}

void ControlBodyTranslationVisitor::processApply(const P4::ApplyMethod* method) {
    auto table = control->getTable(method->object->getName().name);
    BUG_CHECK(table != nullptr, "No table for %1%", method->expr);

    P4::ParameterSubstitution binding;
    binding.populate(method->getActualParameters(), method->expr->arguments);
    cstring actionVariableName;
    if (!saveAction.empty()) {
        actionVariableName = saveAction.at(saveAction.size() - 1);
        if (!actionVariableName.isNullOrEmpty()) {
            builder->appendFormat("enum %s %s;\n", table->actionEnumName, actionVariableName);
            builder->emitIndent();
        }
    }
    builder->blockStart();

    if (!binding.empty()) {
        builder->emitIndent();
        builder->appendLine("/* bind parameters */");
        for (auto p : *method->getActualParameters()->getEnumerator()) {
            toDereference.emplace(p);
            auto etype = EBPFTypeFactory::instance->create(p->type);
            builder->emitIndent();

            bool pointer = p->direction != IR::Direction::In;
            etype->declare(builder, p->name.name, pointer);

            builder->append(" = ");
            auto e = binding.lookup(p);
            if (pointer)
                builder->append("&");
            visit(e);
            builder->endOfStatement(true);
        }
        builder->newline();
    }

    builder->emitIndent();
    builder->appendLine("/* construct key */");
    builder->emitIndent();
    cstring keyname = "key";
    builder->appendFormat("struct %s %s", table->keyTypeName, keyname);
    builder->endOfStatement(true);

    table->createKey(builder, keyname);
    builder->emitIndent();
    builder->appendLine("/* value */");
    builder->emitIndent();
    cstring valueName = "value";
    builder->appendFormat("struct %s *%s", table->valueTypeName, valueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendLine("/* perform lookup */");
    builder->emitIndent();
    builder->target->emitTableLookup(builder, table->dataMapName, keyname, valueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s == NULL) ", valueName);
    builder->blockStart();

    builder->emitIndent();
    builder->appendLine("/* miss; find default action */");
    builder->emitIndent();
    builder->appendFormat("%s = 0", control->hitVariable);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->target->emitTableLookup(builder, table->defaultActionMapName,
                                     control->program->zeroKey, valueName);
    builder->endOfStatement(true);
    builder->blockEnd(false);
    builder->append(" else ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s = 1", control->hitVariable);
    builder->endOfStatement(true);
    builder->blockEnd(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", valueName);
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("/* run action */");
    table->runAction(builder, valueName);
    if (!actionVariableName.isNullOrEmpty()) {
        builder->emitIndent();
        builder->appendFormat("%s = %s->action;\n", actionVariableName, valueName);
    }
    toDereference.clear();

    builder->blockEnd(true);
    builder->blockEnd(true);
}

bool ControlBodyTranslationVisitor::preorder(const IR::ExitStatement*) {
    builder->appendFormat("goto %s;", control->program->endLabel.c_str());
    return false;
}

bool ControlBodyTranslationVisitor::preorder(const IR::ReturnStatement*) {
    builder->appendFormat("goto %s;", control->program->endLabel.c_str());
    return false;
}

bool ControlBodyTranslationVisitor::preorder(const IR::IfStatement* statement) {
    bool isHit = P4::TableApplySolver::isHit(statement->condition, control->program->refMap,
                                             control->program->typeMap);
    if (isHit) {
        // visit first the table, and then the conditional
        auto member = statement->condition->to<IR::Member>();
        CHECK_NULL(member);
        visit(member->expr);  // table application.  Sets 'hitVariable'
        builder->emitIndent();
    }

    // This is almost the same as the base class method
    builder->append("if (");
    if (isHit)
        builder->append(control->hitVariable);
    else
        visit(statement->condition);
    builder->append(") ");
    if (!statement->ifTrue->is<IR::BlockStatement>()) {
        builder->increaseIndent();
        builder->newline();
        builder->emitIndent();
    }
    visit(statement->ifTrue);
    if (!statement->ifTrue->is<IR::BlockStatement>())
        builder->decreaseIndent();
    if (statement->ifFalse != nullptr) {
        builder->newline();
        builder->emitIndent();
        builder->append("else ");
        if (!statement->ifFalse->is<IR::BlockStatement>()) {
            builder->increaseIndent();
            builder->newline();
            builder->emitIndent();
        }
        visit(statement->ifFalse);
        if (!statement->ifFalse->is<IR::BlockStatement>())
            builder->decreaseIndent();
    }
    return false;
}

bool ControlBodyTranslationVisitor::preorder(const IR::SwitchStatement* statement) {
    cstring newName = control->program->refMap->newName("action_run");
    saveAction.push_back(newName);
    // This must be a table.apply().action_run
    auto mem = statement->expression->to<IR::Member>();
    BUG_CHECK(mem != nullptr,
              "%1%: Unexpected expression in switch statement", statement->expression);
    visit(mem->expr);
    saveAction.pop_back();
    saveAction.push_back(nullptr);
    builder->emitIndent();
    builder->append("switch (");
    builder->append(newName);
    builder->append(") ");
    builder->blockStart();
    for (auto c : statement->cases) {
        builder->emitIndent();
        if (c->label->is<IR::DefaultExpression>()) {
            builder->append("default");
        } else {
            builder->append("case ");
            auto pe = c->label->to<IR::PathExpression>();
            auto decl = control->program->refMap->getDeclaration(pe->path, true);
            BUG_CHECK(decl->is<IR::P4Action>(), "%1%: expected an action", pe);
            auto act = decl->to<IR::P4Action>();
            cstring name = nameFromAnnotation(act->annotations, act->name);
            builder->append(name);
        }
        builder->append(":");
        builder->newline();
        builder->emitIndent();
        visit(c->statement);
        builder->newline();
        builder->emitIndent();
        builder->appendLine("break;");
    }
    builder->blockEnd(false);
    saveAction.pop_back();
    return false;
}
}  // namespace

/////////////////////////////////////////////////

EBPFControl::EBPFControl(const EBPFProgram* program,
                         const IR::ControlBlock* block) :
        program(program), controlBlock(block), headers(nullptr), accept(nullptr) {}

bool EBPFControl::build() {
    hitVariable = program->refMap->newName("hit");
    auto pl = controlBlock->container->type->applyParams;
    if (pl->size() != 2) {
        ::error("Expected control block to have exactly 2 parameters");
        return false;
    }

    auto it = pl->parameters->begin();
    headers = *it;
    ++it;
    accept = *it;

    for (auto c : controlBlock->constantValue) {
        auto b = c.second;
        if (!b->is<IR::Block>()) continue;
        if (b->is<IR::TableBlock>()) {
            auto tblblk = b->to<IR::TableBlock>();
            auto tbl = new EBPFTable(program, tblblk);
            tables.emplace(tblblk->container->name, tbl);
        } else if (b->is<IR::ExternBlock>()) {
            auto ctrblk = b->to<IR::ExternBlock>();
            auto node = ctrblk->node;
            if (node->is<IR::Declaration_Instance>()) {
                auto di = node->to<IR::Declaration_Instance>();
                cstring name = nameFromAnnotation(di->annotations, di->getName());
                auto ctr = new EBPFCounterTable(program, ctrblk, name);
                counters.emplace(name, ctr);
            }
        } else {
            ::error("Unexpected block %s nested within control", b->toString());
        }
    }
    return true;
}

void EBPFControl::emitDeclaration(const IR::Declaration* decl, CodeBuilder* builder) {
    if (decl->is<IR::Declaration_Variable>()) {
        auto vd = decl->to<IR::Declaration_Variable>();
        auto etype = EBPFTypeFactory::instance->create(vd->type);
        builder->emitIndent();
        etype->declare(builder, vd->name, false);
        builder->endOfStatement(true);
        BUG_CHECK(vd->initializer == nullptr,
                  "%1%: declarations with initializers not supported", decl);
        return;
    } else if (decl->is<IR::P4Table>() ||
               decl->is<IR::P4Action>() ||
               decl->is<IR::Declaration_Instance>()) {
        return;
    }
    BUG("%1%: not yet handled", decl);
}

void EBPFControl::emit(CodeBuilder* builder) {
    auto hitType = EBPFTypeFactory::instance->create(IR::Type_Boolean::get());
    builder->emitIndent();
    hitType->declare(builder, hitVariable, false);
    builder->endOfStatement(true);
    for (auto a : *controlBlock->container->controlLocals)
        emitDeclaration(a, builder);
    builder->emitIndent();
    ControlBodyTranslationVisitor psi(this, builder);
    controlBlock->container->body->apply(psi);
    builder->newline();
}

void EBPFControl::emitTables(CodeBuilder* builder) {
    for (auto it : tables)
        it.second->emit(builder);
    for (auto it : counters)
        it.second->emit(builder);
}

}  // namespace EBPF
