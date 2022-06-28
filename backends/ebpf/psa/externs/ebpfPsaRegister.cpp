/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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

#include "ebpfPsaRegister.h"
#include "backends/ebpf/psa/ebpfPsaControl.h"

namespace EBPF {

EBPFRegisterPSA::EBPFRegisterPSA(const EBPFProgram *program,
                                 cstring instanceName, const IR::Declaration_Instance *di,
                                 CodeGenInspector *codeGen) : EBPFTableBase(program,
                                                                            instanceName,
                                                                            codeGen) {
    CHECK_NULL(di);
    if (!di->type->is<IR::Type_Specialized>()) {
        ::error(ErrorType::ERR_MODEL, "Missing specialization: %1%", di);
        return;
    }
    auto ts = di->type->to<IR::Type_Specialized>();

    if (ts->arguments->size() != 2) {
        ::error(ErrorType::ERR_MODEL, "Expected a type specialized with two arguments: %1%", ts);
        return;
    }

    this->keyArg = ts->arguments->at(1);
    this->valueArg = ts->arguments->at(0);
    this->keyType = EBPFTypeFactory::instance->create(keyArg);
    this->valueType = EBPFTypeFactory::instance->create(valueArg);

    if (di->arguments->size() < 1) {
        ::error(ErrorType::ERR_MODEL, "Expected at least 1 argument: %1%", di);
        return;
    }

    auto declaredSize = di->arguments->at(0)->expression->to<IR::Constant>();
    CHECK_NULL(declaredSize);

    if (!declaredSize->fitsUint()) {
        ::error(ErrorType::ERR_OVERLIMIT, "%1%: size too large", declaredSize);
        return;
    }
    size = declaredSize->asUnsigned();

    if (di->arguments->size() == 2) {
        if (auto initVal = di->arguments->at(1)->expression->to<IR::Constant>()) {
            this->initialValue = initVal;
        }
    }
}

bool EBPFRegisterPSA::shouldUseArrayMap() {
    CHECK_NULL(this->keyType);
    if (auto wt = dynamic_cast<IHasWidth *>(this->keyType)) {
        unsigned keyWidth = wt->widthInBits();
        // For keys <= 32 bit register is based on array map,
        // otherwise we use hash map
        return (keyWidth > 0 && keyWidth <= 32);
    }

    ::error(ErrorType::ERR_MODEL, "Unexpected key type: %1%", this->keyType->type);

    return false;
}

void EBPFRegisterPSA::emitTypes(CodeBuilder* builder) {
    emitKeyType(builder);
    emitValueType(builder);
}

void EBPFRegisterPSA::emitKeyType(CodeBuilder* builder) {
    builder->emitIndent();
    builder->append("typedef ");
    this->keyType->declare(builder, keyTypeName, false);
    builder->endOfStatement(true);
}

void EBPFRegisterPSA::emitValueType(CodeBuilder* builder) {
    builder->emitIndent();
    if (this->isAtomic) {
        builder->append("typedef struct");
        builder->spc();
        builder->blockStart();
        if (auto type = this->valueType->to<EBPFTypeName>()) {
            if (auto strType = type->toCanonical<EBPFStructType>()){
                for (auto f : strType->fields) {
                    auto fieldType = f->type;
                    builder->emitIndent();
                    fieldType->declare(builder, f->field->name, false);
                    builder->endOfStatement(true);
                }
            }
        } else {
            builder->emitIndent();
            this->valueType->declare(builder, "value", false);
            builder->endOfStatement(true);
        }
        builder->emitIndent();
        builder->append("struct bpf_spin_lock lock");
        builder->endOfStatement(true);
        builder->blockEnd(false);
        builder->spc();
        builder->append(valueTypeName);
    } else {
        builder->append("typedef ");
        this->valueType->declare(builder, valueTypeName, false);
    }
    builder->endOfStatement(true);
}

void EBPFRegisterPSA::emitInitializer(CodeBuilder* builder) {
    if (!shouldUseArrayMap()) {
        // initialize array-based Registers only,
        // hash-based Registers are "lazy-initialized", upon a first lookup to the map.
        return;
    }

    if (this->initialValue == nullptr || this->initialValue->value.is_zero()) {
        // for array maps, initialize only if an initial value is provided by a developer,
        // or if an initial value doesn't equal 0. Otherwise, array map is already zero-initialized.
        return;
    }

    auto ret = program->refMap->newName("ret");
    cstring keyName = program->refMap->newName("key");
    cstring valueName = program->refMap->newName("value");

    builder->emitIndent();
    builder->appendFormat("%s %s", keyTypeName.c_str(), keyName.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("%s %s = ", valueTypeName.c_str(), valueName.c_str());
    builder->append(this->initialValue->value.str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("for (size_t index = 0; index < %u; index++) ", this->size);
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s = index", keyName.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("int %s = ", ret.c_str());
    builder->target->emitTableUpdate(builder, instanceName,
                                     keyName, valueName);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("if (%s) ", ret.c_str());
    builder->blockStart();
    cstring msgStr = Util::printf_format(
            "Map initializer: Error while map (%s) update, code: %s", instanceName, "%d");
    builder->target->emitTraceMessage(builder, msgStr,
                                      1, ret.c_str());

    builder->blockEnd(true);

    builder->blockEnd(true);
}

void EBPFRegisterPSA::emitInstance(CodeBuilder *builder) {
    builder->target->emitTableDecl(builder, instanceName,
                                   shouldUseArrayMap() ? TableArray : TableHash,
                                   this->keyTypeName,
                                   this->valueTypeName, size);
}

void EBPFRegisterPSA::emitRegisterRead(CodeBuilder* builder, const P4::ExternMethod* method,
                                       ControlBodyTranslatorPSA* translator,
                                       const IR::Expression* leftExpression) {
    auto indexArg = method->expr->arguments->at(0)->expression->to<IR::PathExpression>();
    cstring indexParamName = translator->getParamName(indexArg);
    BUG_CHECK(!indexParamName.isNullOrEmpty(), "Index param cannot be empty");

    this->readValueName = program->refMap->newName("value");

    builder->emitIndent();
    this->valueType->declare(builder, readValueName, true);
    builder->endOfStatement(true);

    cstring msgStr = Util::printf_format("Register: reading %s",
                                 instanceName.c_str());
    builder->target->emitTraceMessage(builder, msgStr.c_str());

    builder->emitIndent();
    builder->target->emitTableLookup(builder, dataMapName, indexParamName, readValueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", readValueName.c_str());
    builder->blockStart();
    if (this->isAtomic) {
        builder->emitIndent();
        builder->appendFormat("bpf_spin_lock(((%s *)%s)->lock)",
                              this->valueTypeName, readValueName);
        builder->endOfStatement(true);
    }
    builder->emitIndent();
    if (leftExpression != nullptr) {
        codeGen->visit(leftExpression);
        builder->appendFormat(" = *%s", readValueName);
        builder->endOfStatement(true);
    }
    builder->target->emitTraceMessage(builder, "Register: Entry found!");
    builder->blockEnd(false);
    builder->appendFormat(" else ");
    builder->blockStart();
    builder->emitIndent();

    if (leftExpression != nullptr) {
        codeGen->visit(leftExpression);
        builder->append(" = ");
        if (this->initialValue != nullptr) {
            builder->append(this->initialValue->value.str());
        } else {
            builder->append("(");
            this->valueType->declare(builder, cstring::empty, false);
            builder->append(")");
            this->valueType->emitInitializer(builder);
        }
        builder->endOfStatement(true);
    }

    builder->target->emitTraceMessage(builder, "Register: Entry not found, using default value");
    builder->blockEnd(true);
}

void EBPFRegisterPSA::emitRegisterWrite(CodeBuilder* builder, const P4::ExternMethod* method,
                                        ControlBodyTranslatorPSA* translator) {
    auto indexArgExpr = method->expr->arguments->at(0)->expression->to<IR::PathExpression>();
    cstring indexParamName = translator->getParamName(indexArgExpr);
    auto valueArgExpr = method->expr->arguments->at(1)->expression->to<IR::PathExpression>();
    cstring valueParamName = translator->getParamName(valueArgExpr);
    BUG_CHECK(!indexParamName.isNullOrEmpty(), "Index param cannot be empty");
    BUG_CHECK(!valueParamName.isNullOrEmpty(), "Value param cannot be empty");

    cstring msgStr = Util::printf_format("Register: writing %s",
                                 instanceName.c_str());
    builder->target->emitTraceMessage(builder, msgStr.c_str());

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", readValueName.c_str());
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("*%s = %s", readValueName, valueParamName);
    builder->endOfStatement(true);
    if (this->isAtomic) {
        builder->emitIndent();
        builder->appendFormat("bpf_spin_unlock(((%s *)%s)->lock)",
                              this->valueTypeName, readValueName);
        builder->endOfStatement(true);
    }
    builder->blockEnd(false);
    builder->spc();
    builder->append("else ");
    builder->blockStart();

    cstring valueVar = valueParamName;
    if (isAtomic) {
        builder->emitIndent();
        valueVar = program->refMap->newName("tmp");
        builder->appendFormat("%s %s = {0}", valueTypeName, valueVar);
        builder->endOfStatement(true);
        builder->emitIndent();
        builder->appendFormat("__builtin_memcpy(&%s, &%s, sizeof(%s))", valueVar,
                              valueParamName, valueParamName);
        builder->endOfStatement(true);
    }

    builder->emitIndent();
    auto ret = program->refMap->newName("ret");
    builder->appendFormat("int %s = ", ret.c_str());
    builder->target->emitTableUpdate(builder, instanceName, indexParamName, valueVar);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("if (%s) ", ret.c_str());
    builder->blockStart();
    msgStr = Util::printf_format(
            "Register: Error while map (%s) update, code: %s", instanceName, "%d");
    builder->target->emitTraceMessage(builder, msgStr,
                                      1, ret.c_str());

    builder->blockEnd(true);
    builder->blockEnd(true);
}

}  // namespace EBPF
