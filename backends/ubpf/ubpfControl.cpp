/*
Copyright 2019 Orange

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
#include "ubpfControl.h"

#include "backends/ubpf/ubpfType.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/typeMap.h"
#include "lib/error.h"

namespace UBPF {

UBPFControlBodyTranslator::UBPFControlBodyTranslator(const UBPFControl *control)
    : EBPF::CodeGenInspector(control->program->refMap, control->program->typeMap),
      EBPF::ControlBodyTranslator(control),
      control(control),
      p4lib(P4::P4CoreLibrary::instance()) {
    setName("UBPFControlBodyTranslator");
}

void UBPFControlBodyTranslator::processFunction(const P4::ExternFunction *function) {
    if (function->method->name.name == control->program->model.drop.name) {
        builder->appendFormat("%s = false", control->passVariable);
        return;
    } else if (function->method->name.name == control->program->model.pass.name) {
        builder->appendFormat("%s = true", control->passVariable);
        return;
    } else if (function->method->name.name == control->program->model.ubpf_time_get_ns.name) {
        builder->append(control->program->model.ubpf_time_get_ns.name + "()");
        return;
    } else if (function->method->name.name == control->program->model.csum_replace2.name) {
        processChecksumReplace2(function);
        return;
    } else if (function->method->name.name == control->program->model.csum_replace4.name) {
        processChecksumReplace4(function);
        return;
    } else if (function->method->name.name == control->program->model.hash.name) {
        cstring hashKeyInstanceName = createHashKeyInstance(function);

        auto algorithmTypeArgument = function->expr->arguments->at(1)->expression->to<IR::Member>();
        auto algorithmType = algorithmTypeArgument->member.name;

        if (algorithmType == control->program->model.hashAlgorithm.lookup3.name) {
            builder->appendFormat(" = ubpf_hash(&%s, sizeof(%s))", hashKeyInstanceName,
                                  hashKeyInstanceName);
        } else {
            ::error(ErrorType::ERR_UNSUPPORTED, "%1%: Not supported hash algorithm type",
                    algorithmType);
        }

        return;
    } else if (function->method->name.name == control->program->model.truncate.name) {
        if (function->expr->arguments->size() == 1) {
            builder->appendFormat("%s = (int) (", control->program->packetTruncatedSizeVar.c_str());
            visit(function->expr->arguments->at(0)->expression);
            builder->append(")");
        } else {
            ::error(ErrorType::ERR_EXPECTED, "%1%: One argument expected", function->expr);
        }
        return;
    }
    processCustomExternFunction(function, UBPFTypeFactory::instance);
}

void UBPFControlBodyTranslator::processChecksumReplace2(const P4::ExternFunction *function) {
    builder->append(control->program->model.csum_replace2.name + "(");
    auto v = function->expr->arguments;
    bool first = true;
    for (auto arg : *v) {
        if (!first) builder->append(", ");
        first = false;
        visit(arg);
    }
    builder->append(")");
}

void UBPFControlBodyTranslator::processChecksumReplace4(const P4::ExternFunction *function) {
    builder->append(control->program->model.csum_replace4.name + "(");
    auto v = function->expr->arguments;
    bool first = true;
    for (auto arg : *v) {
        if (!first) builder->append(", ");
        first = false;
        visit(arg);
    }
    builder->append(")");
}

cstring UBPFControlBodyTranslator::createHashKeyInstance(const P4::ExternFunction *function) {
    auto dataArgument = function->expr->arguments->at(2)->expression->to<IR::ListExpression>();

    auto atype = UBPFTypeFactory::instance->create(dataArgument->type);
    if (!atype->is<UBPFListType>()) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%: Unsupported argument type",
                dataArgument->type);
    }
    auto ubpfList = atype->to<UBPFListType>();
    ubpfList->name = this->refMap->newName("tuple");

    atype->declare(builder, "", false);
    builder->blockStart();
    atype->emit(builder);
    builder->blockEnd(false);
    builder->endOfStatement(true);

    auto hashKeyInstance = control->program->refMap->newName("hash_key_instance");
    builder->emitIndent();
    atype->declare(builder, hashKeyInstance, false);
    builder->append(" = ");
    atype->emitInitializer(builder);
    builder->endOfStatement(true);
    unsigned idx = 0;
    for (auto f : ubpfList->elements) {
        if (!f->type)  // If nullptr it's a padding.
            continue;
        builder->emitIndent();
        builder->appendFormat("%s.%s = ", hashKeyInstance, f->name);
        auto c = dataArgument->components.at(idx);
        visit(c);
        builder->endOfStatement(true);
        idx++;
    }

    auto destination = function->expr->arguments->at(0);
    builder->emitIndent();
    visit(destination);
    return hashKeyInstance;
}

void UBPFControlBodyTranslator::processMethod(const P4::ExternMethod *method) {
    auto decl = method->object;
    auto declType = method->originalExternType;

    if (declType->name.name == p4lib.packetOut.name) {
        if (method->method->name.name == p4lib.packetOut.emit.name) {
            ::error(ErrorType::ERR_UNSUPPORTED, "%1%: Emit extern not supported in control block",
                    method->expr);
            return;
        }
    } else if (declType->name.name == UBPFModel::instance.registerModel.name) {
        cstring name = decl->getName().name;
        auto pRegister = control->getRegister(name);

        if (method->method->name.name == UBPFModel::instance.registerModel.write.name) {
            pRegister->emitKeyInstance(builder, method->expr);
        }

        pRegister->emitMethodInvocation(builder, method);
        return;
    }
    ::error(ErrorType::ERR_UNEXPECTED, "%1%: Unexpected method call", method->expr);
}

void UBPFControlBodyTranslator::processApply(const P4::ApplyMethod *method) {
    auto table = control->getTable(method->object->getName().name);
    BUG_CHECK(table != nullptr, "No table for %1%", method->expr);

    cstring actionVariableName;
    if (!saveAction.empty()) {
        actionVariableName = saveAction.at(saveAction.size() - 1);
        if (!actionVariableName.isNullOrEmpty()) {
            builder->appendFormat("enum %s %s;\n", table->actionEnumName.c_str(),
                                  actionVariableName.c_str());
            builder->emitIndent();
        }
    }
    builder->blockStart();

    BUG_CHECK(method->expr->arguments->empty(), "%1%: table apply with arguments", method);
    cstring keyname = "key";
    if (table->keyGenerator != nullptr) {
        builder->emitIndent();
        builder->appendLine("/* construct key */");
        builder->emitIndent();
        builder->appendFormat("struct %s %s = {}", table->keyTypeName.c_str(), keyname.c_str());
        builder->endOfStatement(true);
        table->emitKey(builder, keyname);
    }
    builder->emitIndent();
    builder->appendLine("/* value */");
    builder->emitIndent();
    cstring valueName = "value";
    builder->appendFormat("struct %s *%s = NULL", table->valueTypeName.c_str(), valueName.c_str());
    builder->endOfStatement(true);

    if (table->keyGenerator != nullptr) {
        builder->emitIndent();
        builder->appendLine("/* perform lookup */");
        builder->emitIndent();
        builder->appendFormat("%s = ", valueName.c_str());
        builder->target->emitTableLookup(builder, table->dataMapName, keyname, valueName);
        builder->endOfStatement(true);
    }

    builder->newline();

    builder->emitIndent();
    builder->appendFormat("if (%s == NULL) ", valueName.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendLine("/* miss; find default action */");
    builder->emitIndent();
    builder->appendFormat("%s = 0", control->hitVariable.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->append("value = ");
    builder->target->emitTableLookup(builder, table->defaultActionMapName,
                                     control->program->zeroKey, valueName);
    builder->endOfStatement(true);
    builder->blockEnd(false);
    builder->append(" else ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s = 1", control->hitVariable.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", valueName.c_str());
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("/* run action */");
    table->emitAction(builder, valueName);
    if (!actionVariableName.isNullOrEmpty()) {
        builder->emitIndent();
        builder->appendFormat("%s = %s->action", actionVariableName.c_str(), valueName.c_str());
        builder->endOfStatement(true);
    }
    toDereference.clear();

    builder->blockEnd(true);
    builder->emitIndent();
    builder->appendFormat("else return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);

    builder->blockEnd(true);
}

bool UBPFControlBodyTranslator::preorder(const IR::PathExpression *expression) {
    auto decl = control->program->refMap->getDeclaration(expression->path, true);
    auto param = decl->getNode()->to<IR::Parameter>();
    if (param != nullptr) {
        if (toDereference.count(param) > 0) builder->append("*");
        auto subst = ::get(substitution, param);
        if (subst != nullptr) {
            builder->append(subst->name);
            return false;
        }
    }
    builder->append(expression->path->name);  // each identifier should be unique
    return false;
}

bool UBPFControlBodyTranslator::preorder(const IR::MethodCallStatement *s) {
    visit(s->methodCall);
    builder->endOfStatement(true);
    return false;
}

bool UBPFControlBodyTranslator::preorder(const IR::MethodCallExpression *expression) {
    auto mi = P4::MethodInstance::resolve(expression, control->program->refMap,
                                          control->program->typeMap);
    if (auto apply = mi->to<P4::ApplyMethod>()) {
        processApply(apply);
        return false;
    }

    if (auto ef = mi->to<P4::ExternFunction>()) {
        processFunction(ef);
        return false;
    }

    if (auto ext = mi->to<P4::ExternMethod>()) {
        processMethod(ext);
        return false;
    }

    if (auto bim = mi->to<P4::BuiltInMethod>()) {
        if (bim->name == IR::Type_Header::isValid) {
            visit(bim->appliedTo);
            builder->append(".ebpf_valid");
            return false;
        } else if (bim->name == IR::Type_Header::setValid) {
            builder->emitIndent();
            visit(bim->appliedTo);
            builder->append(".ebpf_valid = true");
            return false;
        } else if (bim->name == IR::Type_Header::setInvalid) {
            builder->emitIndent();
            visit(bim->appliedTo);
            builder->append(".ebpf_valid = false");
            return false;
        }
    }

    if (auto ac = mi->to<P4::ActionCall>()) {
        // Action arguments have been eliminated by the mid-end.
        BUG_CHECK(expression->arguments->empty(), "%1%: unexpected arguments for action call",
                  expression);
        visit(ac->action->body);
        return false;
    }

    ::error(ErrorType::ERR_UNEXPECTED, "Unsupported method invocation %1%", expression);
    return false;
}

bool UBPFControlBodyTranslator::preorder(const IR::AssignmentStatement *a) {
    if (a->right->is<IR::MethodCallExpression>()) {
        auto method = a->right->to<IR::MethodCallExpression>();
        if (method->method->is<IR::Member>()) {
            auto methodName = method->method->to<IR::Member>()->member.name;
            if (methodName == UBPFModel::instance.registerModel.read.name) {
                return emitRegisterRead(a, method);
            }
        }
    }

    emitAssignmentStatement(a);

    return false;
}

bool UBPFControlBodyTranslator::emitRegisterRead(const IR::AssignmentStatement *a,
                                                 const IR::MethodCallExpression *method) {
    auto pathExpr = method->method->to<IR::Member>()->expr->to<IR::PathExpression>();
    auto registerName = pathExpr->path->name.name;
    auto pRegister = control->getRegister(registerName);
    pRegister->emitKeyInstance(builder, method);

    auto etype = UBPFTypeFactory::instance->create(pRegister->keyType);
    auto tmp = control->program->refMap->newName("tmp");
    etype->declare(builder, tmp, true);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->append(tmp);
    builder->append(" = ");
    visit(a->right);
    builder->endOfStatement();

    builder->newline();
    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", tmp);
    builder->blockStart();

    builder->emitIndent();
    visit(a->left);
    builder->append(" = *");
    builder->append(tmp);
    builder->endOfStatement();

    registersLookups.push_back(pRegister);

    return false;
}

void UBPFControlBodyTranslator::emitAssignmentStatement(const IR::AssignmentStatement *a) {
    visit(a->left);
    builder->append(" = ");
    visit(a->right);
    builder->endOfStatement();
}

bool UBPFControlBodyTranslator::preorder(const IR::BlockStatement *s) {
    builder->blockStart();
    for (auto a : s->components) {
        builder->newline();
        builder->emitIndent();
        visit(a);
    }
    if (!s->components.empty()) builder->newline();
    builder->blockEnd(false);
    return false;
}

bool UBPFControlBodyTranslator::preorder(const IR::ExitStatement *) {
    builder->appendFormat("goto %s;", control->program->endLabel.c_str());
    return false;
}

bool UBPFControlBodyTranslator::preorder(const IR::ReturnStatement *) {
    builder->appendFormat("goto %s;", control->program->endLabel.c_str());
    return false;
}

bool UBPFControlBodyTranslator::preorder(const IR::IfStatement *statement) {
    bool isHit = P4::TableApplySolver::isHit(statement->condition, control->program->refMap,
                                             control->program->typeMap);
    if (isHit) {
        // visit first the table, and then the conditional
        auto member = statement->condition->to<IR::Member>();
        CHECK_NULL(member);
        visit(member->expr);  // table application.  Sets 'hitVariable'
        builder->emitIndent();
    }

    builder->append("if (");
    if (isHit)
        builder->append(control->hitVariable);
    else
        visit(statement->condition);
    builder->append(") ");
    if (!statement->ifTrue->is<IR::BlockStatement>()) {
        builder->blockStart();
        builder->emitIndent();
    }
    visit(statement->ifTrue);
    if (!statement->ifTrue->is<IR::BlockStatement>()) {
        builder->blockEnd(statement->ifFalse == nullptr);
    }
    if (statement->ifFalse != nullptr) {
        builder->append(" else ");
        if (!statement->ifFalse->is<IR::BlockStatement>()) {
            builder->blockStart();
            builder->emitIndent();
        }
        visit(statement->ifFalse);
        if (!statement->ifFalse->is<IR::BlockStatement>()) {
            builder->blockEnd(true);
        }
    }
    return false;
}

bool UBPFControlBodyTranslator::preorder(const IR::SwitchStatement *statement) {
    cstring newName = control->program->refMap->newName("action_run");
    saveAction.push_back(newName);
    // This must be a table.apply().action_run
    auto mem = statement->expression->to<IR::Member>();
    BUG_CHECK(mem != nullptr, "%1%: Unexpected expression in switch statement",
              statement->expression);
    visit(mem->expr);
    saveAction.pop_back();
    saveAction.emplace_back(nullptr);
    builder->emitIndent();
    builder->append("switch (");
    builder->append(newName);
    builder->append(") ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("/* Switch statement start block */\n");
    for (auto c : statement->cases) {
        builder->emitIndent();
        builder->appendLine("/* Statement case */");
        builder->emitIndent();
        if (c->label->is<IR::DefaultExpression>()) {
            builder->append("default");
        } else {
            builder->append("case ");
            auto pe = c->label->to<IR::PathExpression>();
            auto decl = control->program->refMap->getDeclaration(pe->path, true);
            BUG_CHECK(decl->is<IR::P4Action>(), "%1%: expected an action", pe);
            auto act = decl->to<IR::P4Action>();
            auto tblName = mem->to<IR::Operation_Unary>()
                               ->expr->to<IR::Expression>()
                               ->type->to<IR::Type_StructLike>()
                               ->getName()
                               .name;
            auto table = control->getTable(tblName);
            cstring name = table->generateActionName(act);
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

bool UBPFControlBodyTranslator::preorder(const IR::Operation_Binary *b) {
    widthCheck(b);
    builder->append("(");
    visit(b->left);
    builder->spc();
    builder->append(b->getStringOp());
    builder->spc();
    visit(b->right);
    builder->append(")");
    return false;
}

bool UBPFControlBodyTranslator::comparison(const IR::Operation_Relation *b) {
    auto type = typeMap->getType(b->left);
    auto et = UBPFTypeFactory::instance->create(type);

    bool scalar = (et->is<UBPFScalarType>() &&
                   UBPFScalarType::generatesScalar(et->to<UBPFScalarType>()->widthInBits())) ||
                  et->is<UBPFBoolType>();
    if (scalar) {
        builder->append("(");
        visit(b->left);
        builder->spc();
        builder->append(b->getStringOp());
        builder->spc();
        visit(b->right);
        builder->append(")");
    } else {
        if (!et->is<EBPF::IHasWidth>())
            BUG("%1%: Comparisons for type %2% not yet implemented", type);
        unsigned width = et->to<EBPF::IHasWidth>()->implementationWidthInBits();
        builder->append("memcmp(&");
        visit(b->left);
        builder->append(", &");
        visit(b->right);
        builder->appendFormat(", %d)", width / 8);
    }
    return false;
}

bool UBPFControlBodyTranslator::preorder(const IR::Member *expression) {
    cstring name = "";
    if (expression->expr->is<IR::PathExpression>()) {
        name = expression->expr->to<IR::PathExpression>()->path->name.name;
    }
    auto ei = P4::EnumInstance::resolve(expression, typeMap);
    if (ei == nullptr) {
        visit(expression->expr);
        if (name == control->program->stdMetadataVar) {
            builder->append("->");
        } else {
            builder->append(".");
        }
    }
    builder->append(expression->member);
    return false;
}

UBPFControl::UBPFControl(const UBPFProgram *program, const IR::ControlBlock *block,
                         const IR::Parameter *parserHeaders)
    : EBPF::EBPFControl(program, block, parserHeaders),
      program(program),
      controlBlock(block),
      headers(nullptr),
      parserHeaders(parserHeaders),
      codeGen(nullptr) {}

void UBPFControl::scanConstants() {
    for (auto c : controlBlock->constantValue) {
        auto b = c.second;
        if (b->is<IR::TableBlock>()) {
            auto tblblk = b->to<IR::TableBlock>();
            auto tbl = new UBPFTable(program, tblblk, codeGen);
            tables.emplace(tblblk->container->name, tbl);
        } else if (b->is<IR::ExternBlock>()) {
            auto ctrblk = b->to<IR::ExternBlock>();
            auto node = ctrblk->node;
            if (node->is<IR::Declaration_Instance>()) {
                auto di = node->to<IR::Declaration_Instance>();
                auto type = di->type->to<IR::Type_Specialized>();
                auto externTypeName = type->baseType->path->name.name;
                if (externTypeName == UBPFModel::instance.registerModel.name) {
                    cstring name = di->name.name;
                    auto ctr = new UBPFRegister(program, ctrblk, name, codeGen);
                    registers.emplace(name, ctr);
                }
            }
        } else if (!b->is<IR::Block>()) {
            continue;
        } else {
            ::error(ErrorType::ERR_UNEXPECTED, "Unexpected block %s nested within control",
                    b->toString());
        }
    }
}

void UBPFControl::emit(EBPF::CodeBuilder *builder) {
    for (auto a : controlBlock->container->controlLocals) {
        emitDeclaration(builder, a);
    }

    codeGen->setBuilder(builder);
    builder->emitIndent();
    controlBlock->container->body->apply(*codeGen);

    for (size_t i = 0; i < codeGen->registersLookups.size(); i++) {
        builder->newline();
        builder->blockEnd(true);
    }
}

void UBPFControl::emitDeclaration(EBPF::CodeBuilder *builder, const IR::Declaration *decl) {
    if (decl->is<IR::Declaration_Variable>()) {
        auto vd = decl->to<IR::Declaration_Variable>();
        auto etype = UBPFTypeFactory::instance->create(vd->type);
        builder->emitIndent();
        etype->declareInit(builder, vd->name, false);
        builder->endOfStatement(true);
        BUG_CHECK(vd->initializer == nullptr, "%1%: declarations with initializers not supported",
                  decl);
        return;
    } else if (decl->is<IR::P4Table>() || decl->is<IR::P4Action>() ||
               decl->is<IR::Declaration_Instance>()) {
        return;
    }
    BUG("%1%: not yet handled", decl);
}

void UBPFControl::emitTableTypes(EBPF::CodeBuilder *builder) {
    for (auto it : tables) it.second->emitTypes(builder);
}

void UBPFControl::emitTableInstances(EBPF::CodeBuilder *builder) {
    for (auto it : tables) it.second->emitInstance(builder);
    for (auto it : registers) it.second->emitInstance(builder);
}

void UBPFControl::emitTableInitializers(EBPF::CodeBuilder *builder) {
    for (auto it : tables) it.second->emitInitializer(builder);
}

bool UBPFControl::build() {
    hitVariable = program->refMap->newName("hit");
    passVariable = program->refMap->newName("pass");
    auto pl = controlBlock->container->type->applyParams;
    size_t numberOfArgs = UBPFModel::instance.numberOfControlBlockArguments();
    if (pl->size() != numberOfArgs) {
        ::error(ErrorType::ERR_EXPECTED, "Expected control block to have exactly %d parameter",
                numberOfArgs);
        return false;
    }

    auto it = pl->parameters.begin();
    headers = *it;

    codeGen = new UBPFControlBodyTranslator(this);
    codeGen->substitute(headers, parserHeaders);

    scanConstants();
    return ::errorCount() == 0;
}

}  // namespace UBPF
