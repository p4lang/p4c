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
#include "ebpfPsaChecksum.h"
#include "ebpfPsaHashAlgorithm.h"

namespace EBPF {

void EBPFChecksumPSA::init(const EBPFProgram* program, cstring name, int type) {
    engine = EBPFHashAlgorithmTypeFactoryPSA::instance()->create(type, program, name);

    if (engine == nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED, "Hash algorithm not yet implemented: %1%",
                declaration->arguments->at(0));
    }
}

EBPFChecksumPSA::EBPFChecksumPSA(const EBPFProgram* program, const IR::Declaration_Instance* block,
                                 cstring name) :
        engine(nullptr), declaration(block) {
    auto di = block->to<IR::Declaration_Instance>();
    if (di->arguments->size() != 1) {
        ::error(ErrorType::ERR_UNEXPECTED, "Expected exactly 1 argument %1%", block);
        return;
    }
    int type = di->arguments->at(0)->expression->checkedTo<IR::Constant>()->asInt();
    init(program, name, type);
}

EBPFChecksumPSA::EBPFChecksumPSA(const EBPFProgram* program, const IR::Declaration_Instance* block,
                                 cstring name, int type) :
        engine(nullptr), declaration(block) {
    init(program, name, type);
}

void EBPFChecksumPSA::processMethod(CodeBuilder* builder, cstring method,
                                    const IR::MethodCallExpression * expr, Visitor * visitor) {
    if (engine == nullptr)
        return;

    engine->setVisitor(visitor);

    if (method == "clear") {
        engine->emitClear(builder);
    } else if (method == "update") {
        engine->emitAddData(builder, 0, expr);
    } else if (method == "get") {
        engine->emitGet(builder);
    } else {
        ::error(ErrorType::ERR_UNEXPECTED, "Unexpected method call %1%", expr);
    }
}

}  // namespace EBPF
