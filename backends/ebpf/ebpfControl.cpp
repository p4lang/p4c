#include "ebpfControl.h"
#include "ebpfType.h"
#include "ebpfTable.h"
#include "../../frontends/common/typeMap.h"
#include "../../frontends/p4/methodInstance.h"
#include "ir/parameterSubstitution.h"

namespace EBPF {

// TODO: table applys should be compiled into separate inline functions,
// and then we can generate calls as proper expressions.
// Since then if (table.apply().hit) { ... }
// is broken.
class ControlTranslationVisitor : public CodeGenInspector {
    const EBPFControl* control;
    std::set<const IR::Parameter*> toDereference;
    std::vector<cstring> saveAction;

 public:
    ControlTranslationVisitor(const EBPFControl* control, CodeBuilder* builder) :
            CodeGenInspector(builder, control->program->blockMap->typeMap), control(control) {}
    using CodeGenInspector::preorder;
    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::MethodCallStatement* stat) override
    { saveAction.push_back(nullptr); visit(stat->methodCall); saveAction.pop_back(); return false; }
    bool preorder(const IR::SwitchStatement* stat) override;
    // bool preorder(const IR::IfStatement* stat) override;  // TODO
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool preorder(const IR::ReturnStatement* stat) override;
    void processMethod(const P4::ExternMethod* method);
    void processApply(const P4::ApplyMethod* method);
};

bool ControlTranslationVisitor::preorder(const IR::PathExpression* expression) {
    auto decl = control->program->refMap->getDeclaration(expression->path, true);
    auto param = decl->getNode()->to<IR::Parameter>();
    if (param != nullptr && toDereference.count(param) > 0)
        builder->append("*");
    builder->append(expression->path->toString());
    return false;
}

bool ControlTranslationVisitor::preorder(const IR::MethodCallExpression* expression) {
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

void ControlTranslationVisitor::processMethod(const P4::ExternMethod* method) {
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
    auto counterMap = control->getCounter(decl->name);
    counterMap->emitMethodInvocation(builder, method);
    builder->blockEnd(true);
}

void ControlTranslationVisitor::processApply(const P4::ApplyMethod* method) {
    auto table = control->getTable(method->object->getName().name);
    BUG_CHECK(table != nullptr, "No table for %1%", method->expr);

    IR::ParameterSubstitution binding;
    binding.populate(method->getParameters(), method->expr->arguments);
    cstring actionVariableName = saveAction.at(saveAction.size() - 1);
    if (!actionVariableName.isNullOrEmpty()) {
        builder->appendFormat("%s %s;\n", table->actionEnumName, actionVariableName);
        builder->emitIndent();
    }
    builder->blockStart();

    if (!binding.empty()) {
        builder->emitIndent();
        builder->appendLine("/* bind parameters */");
        for (auto p : *method->getParameters()->getEnumerator()) {
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
    builder->target->emitTableLookup(builder, table->defaultActionMapName,
                                     control->program->zeroKey, valueName);
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
        builder->appendFormat("%s = valueName->action;\n", actionVariableName);
    }
    toDereference.clear();

    builder->blockEnd(true);
    builder->blockEnd(true);
}

bool ControlTranslationVisitor::preorder(const IR::ReturnStatement* stat) {
    BUG_CHECK(stat->expr == nullptr, "%1%: Return statement should have no expression", stat);
    builder->appendFormat("goto %s;", control->program->endLabel.c_str());
    return false;
}

bool ControlTranslationVisitor::preorder(const IR::SwitchStatement* statement) {
    cstring newName = control->program->blockMap->refMap->newName("action_run");
    saveAction.push_back(newName);
    // This must be a table.apply().action_run
    auto mem = statement->expression->to<IR::Member>();
    BUG_CHECK(mem != nullptr,
              "%1%: Unexpected expression in switch statement", statement->expression);
    visit(mem->expr);
    saveAction.pop_back();
    builder->emitIndent();
    builder->append("switch (");
    builder->append(newName);
    builder->append(") ");
    builder->blockStart();
    for (auto c : *statement->cases) {
        builder->emitIndent();
        if (c->label != IR::SwitchStatement::default_label)
            builder->append("case ");
        builder->append(c->label);
        builder->append(":");
        builder->newline();
        builder->emitIndent();
        visit(c->statement);
        builder->newline();
        builder->emitIndent();
        builder->appendLine("break;");
    }
    builder->blockEnd(false);
    return false;
}

/////////////////////////////////////////////////

EBPFControl::EBPFControl(const EBPFProgram* program,
                         const IR::ControlBlock* block) :
        program(program), controlBlock(block) {}

bool EBPFControl::build() {
    auto pl = controlBlock->container->type->applyParams;
    if (pl->size() != 2) {
        ::error("Expected control block to have exactly 2 parameters");
        return false;
    }

    auto it = pl->parameters.begin();
    headers = it->second;
    ++it;
    accept = it->second;

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
                cstring name = node->to<IR::Declaration_Instance>()->name;
                auto ctr = new EBPFCounterTable(program, ctrblk, name);
                counters.emplace(name, ctr);
            }
        } else {
            ::error("Unexpected block %s nested within control", b->toString());
        }
    }
    return true;
}

void EBPFControl::emit(CodeBuilder* builder) {
    builder->emitIndent();
    ControlTranslationVisitor psi(this, builder);
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
