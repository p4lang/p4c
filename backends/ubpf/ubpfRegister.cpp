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

#include "ubpfRegister.h"

#include <string>
#include <vector>

#include "backends/ebpf/ebpfType.h"
#include "backends/ebpf/target.h"
#include "backends/ubpf/target.h"
#include "backends/ubpf/ubpfModel.h"
#include "backends/ubpf/ubpfTable.h"
#include "backends/ubpf/ubpfType.h"
#include "frontends/common/model.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/id.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"

namespace UBPF {

static cstring last_key_name;

UBPFRegister::UBPFRegister(const UBPFProgram *program, const IR::ExternBlock *block, cstring name,
                           EBPF::CodeGenInspector *codeGen)
    : UBPFTableBase(program, name, codeGen) {
    auto di = block->node->to<IR::Declaration_Instance>();
    auto type = program->typeMap->getType(di, true)->to<IR::Type_SpecializedCanonical>();
    auto valueType = type->arguments->operator[](0);
    auto keyType = type->arguments->operator[](1);

    this->valueType = valueType;
    this->keyType = keyType;

    if (valueType->is<IR::Type_Name>()) {
        valueTypeName = valueType->to<IR::Type_Name>()->path->name.name;
    }
    if (keyType->is<IR::Type_Name>()) {
        keyTypeName = keyType->to<IR::Type_Name>()->path->name.name;
    }

    auto sz = block->getParameterValue(program->model.registerModel.sizeParam.name);
    if (sz == nullptr || !sz->is<IR::Constant>()) {
        error(ErrorType::ERR_MODEL,
              "Expected an integer argument for parameter %1% or %2%; is the model corrupted?",
              program->model.registerModel.sizeParam.name, name);
        return;
    }
    auto cst = sz->to<IR::Constant>();
    if (!cst->fitsInt()) {
        error(ErrorType::ERR_OVERLIMIT, "%1%: size too large", cst);
        return;
    }
    size = cst->asInt();
    if (size <= 0) {
        error(ErrorType::ERR_UNEXPECTED, "%1%: negative size", cst);
        return;
    }
}

void UBPFRegister::emitInstance(EBPF::CodeBuilder *builder) {
    UBPFTableBase::emitInstance(builder, EBPF::TableHash);
}

void UBPFRegister::emitMethodInvocation(EBPF::CodeBuilder *builder,
                                        const P4::ExternMethod *method) {
    if (method->method->name.name == program->model.registerModel.read.name) {
        emitRegisterRead(builder, method->expr);
        return;
    } else if (method->method->name.name == program->model.registerModel.write.name) {
        emitRegisterWrite(builder, method->expr);
        return;
    }
    error(ErrorType::ERR_UNEXPECTED, "%1%: Unexpected method for %2%", method->expr,
          program->model.registerModel.read.name);
}

void UBPFRegister::emitRegisterWrite(EBPF::CodeBuilder *builder,
                                     const IR::MethodCallExpression *expression) {
    BUG_CHECK(expression->arguments->size() == 2, "Expected just 2 argument for %1%", expression);

    auto arg_value = expression->arguments->at(1);
    auto target = reinterpret_cast<const UbpfTarget *>(builder->target);

    cstring valueVariableName = emitValueInstanceIfNeeded(builder, arg_value);
    if (valueVariableName == nullptr) {
        valueVariableName = arg_value->expression->toString();
    }
    target->emitTableUpdate(builder, dataMapName, last_key_name, "&" + valueVariableName);
}

cstring UBPFRegister::emitValueInstanceIfNeeded(EBPF::CodeBuilder *builder,
                                                const IR::Argument *arg_value) {
    cstring valueVariableName = nullptr;

    if (arg_value->expression->is<IR::Constant>() ||
        arg_value->expression->is<IR::Operation_Binary>() ||
        arg_value->expression->is<IR::Member>()) {
        auto scalarInstance = UBPFTypeFactory::instance->create(valueType);
        valueVariableName = program->refMap->newName("value_local_var");
        scalarInstance->declare(builder, valueVariableName, false);
        builder->append(" = ");
        codeGen->visit(arg_value->expression);
        builder->endOfStatement(true);
        builder->emitIndent();
    }

    return valueVariableName;
}

void UBPFRegister::emitRegisterRead(EBPF::CodeBuilder *builder,
                                    const IR::MethodCallExpression *expression) {
    BUG_CHECK(expression->arguments->size() == 1, "Expected 1 argument for %1%", expression);
    auto target = builder->target;

    target->emitTableLookup(builder, dataMapName, last_key_name, "");
}

void UBPFRegister::emitKeyInstance(EBPF::CodeBuilder *builder,
                                   const IR::MethodCallExpression *expression) {
    auto arg_key = expression->arguments->at(0);

    cstring keyName = "";

    if (arg_key->expression->is<IR::PathExpression>()) {
        keyName = arg_key->expression->to<IR::PathExpression>()->path->name.name;
    }

    auto type = arg_key->expression->type;
    if (type->is<IR::Type_Bits>()) {
        auto keyType = type->to<IR::Type_Bits>();
        auto tb = keyType->to<IR::Type_Bits>();
        auto scalarType = new UBPFScalarType(tb);
        auto scalarInstance = UBPFTypeFactory::instance->create(scalarType->type);
        keyName = program->refMap->newName("key_local_var");
        scalarInstance->declare(builder, keyName, false);
        builder->append(" = ");
        codeGen->visit(arg_key->expression);
        builder->endOfStatement(true);
        builder->emitIndent();
    } else if (type->is<IR::Type_StructLike>()) {
        keyName = arg_key->expression->to<IR::PathExpression>()->path->name.name;
    }

    last_key_name = keyName;
}

}  // namespace UBPF
