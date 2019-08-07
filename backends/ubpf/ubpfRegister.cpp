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

namespace UBPF {

    static int const_value_counter = 0;

    UBPFRegister::UBPFRegister(const UBPFProgram *program,
                               const IR::ExternBlock *block,
                               cstring name, EBPF::CodeGenInspector *codeGen) :
            UBPFTableBase(program, name, codeGen) {

        auto di = block->node->to<IR::Declaration_Instance>();
        auto type = di->type->to<IR::Type_Specialized>();
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

        auto sz = block->getParameterValue(
                program->model.registerModel.sizeParam.name);
        if (sz == nullptr || !sz->is<IR::Constant>()) {
            error("Expected an integer argument for parameter %1% or %2%; is the model corrupted?",
                  program->model.registerModel.sizeParam.name, name);
            return;
        }
        auto cst = sz->to<IR::Constant>();
        if (!cst->fitsInt()) {
            error("%1%: size too large", cst);
            return;
        }
        size = cst->asInt();
        if (size <= 0) {
            error("%1%: negative size", cst);
            return;
        }
    }

    void
    UBPFRegister::emitMethodInvocation(EBPF::CodeBuilder *builder,
                                       const P4::ExternMethod *method,
                                       const std::vector<cstring> pointerVariables) {
        if (method->method->name.name ==
            program->model.registerModel.read.name) {
            emitRegisterRead(builder, method->expr);
            return;
        } else if (method->method->name.name ==
                   program->model.registerModel.write.name) {
            emitRegisterWrite(builder, method->expr, pointerVariables);
            return;
        }
        error("%1%: Unexpected method for %2%", method->expr,
              program->model.registerModel.read.name);
    }

    void UBPFRegister::emitRegisterWrite(EBPF::CodeBuilder *builder,
                                         const IR::MethodCallExpression *expression,
                                         const std::vector<cstring> pointerVariables) {
        BUG_CHECK(expression->arguments->size() == 2,
                  "Expected just 2 argument for %1%", expression);

        auto arg_value = expression->arguments->at(1);
        auto target = (UbpfTarget *) builder->target;
        cstring keyName = "";
        auto arg_key = expression->arguments->at(0);

        if (arg_key->expression->is<IR::PathExpression>()) {
            keyName = arg_key->expression->to<IR::PathExpression>()->path->name.name;
        } else {
            keyName =
                    const_value_counter == 0 ? "const_value" : "const_value_" +
                                                               std::to_string(
                                                                       const_value_counter -
                                                                       1);
        }

        const_value_counter++;
        if (arg_value->expression->is<IR::PathExpression>()) {
            auto name = arg_value->expression->to<IR::PathExpression>()->path->name.name;
            if (std::find(
                    pointerVariables.begin(),
                    pointerVariables.end(),
                    name) ==
                pointerVariables.end()) {

                target->emitTableUpdate(builder, dataMapName, keyName,
                                        "&" + name);
                return;
            }
        }

        if (arg_value->expression->is<IR::Constant>()) {
            auto scalarInstance = UBPFTypeFactory::instance->create(
                    valueType);
            auto scalarName = program->refMap->newName(
                    "tmp_value");
            scalarInstance->declare(builder, scalarName, false);
            builder->append(" = ");
            codeGen->visit(arg_value->expression);
            builder->endOfStatement(true);
            builder->emitIndent();

            target->emitTableUpdate(builder, dataMapName, keyName,
                                    "&" + scalarName);
            return;
        }

        if (arg_value->expression->is<IR::Operation_Binary>()) {
            auto scalarInstance = UBPFTypeFactory::instance->create(
                    valueType);
            auto scalarName = program->refMap->newName(
                    "tmp_value");
            scalarInstance->declare(builder, scalarName, false);
            builder->append(" = ");
            codeGen->visit(arg_value->expression);
            builder->endOfStatement(true);
            builder->emitIndent();

            target->emitTableUpdate(builder, dataMapName, keyName,
                                    "&" + scalarName);
            return;
        }

        target->emitTableUpdate(codeGen, builder, dataMapName, keyName,
                                arg_value->expression);
    }

    void UBPFRegister::emitRegisterRead(EBPF::CodeBuilder *builder,
                                        const IR::MethodCallExpression *expression) {
        BUG_CHECK(expression->arguments->size() == 1,
                  "Expected 1 argument for %1%", expression);
        auto target = (UbpfTarget *) builder->target;
        cstring keyName = "";
        auto arg_key = expression->arguments->at(0);

        if (arg_key->expression->is<IR::PathExpression>()) {
            keyName = arg_key->expression->to<IR::PathExpression>()->path->name.name;
        } else {
            keyName =
                    const_value_counter == 0 ? "const_value" : "const_value_" +
                                                               std::to_string(
                                                                       const_value_counter -
                                                                       1);
        }

        const_value_counter++;
        target->emitTableLookup(builder, dataMapName, keyName, "");
    }
}


