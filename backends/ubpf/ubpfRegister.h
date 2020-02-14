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

#ifndef P4C_UBPFREGISTER_H
#define P4C_UBPFREGISTER_H

#include "ubpfTable.h"
#include "ubpfType.h"

namespace UBPF {

    class UBPFRegister final : public UBPFTableBase {
    public:
        UBPFRegister(const UBPFProgram *program, const IR::ExternBlock *block,
                     cstring name, EBPF::CodeGenInspector *codeGen);

        void emitRegisterRead(EBPF::CodeBuilder *builder,
                              const IR::MethodCallExpression *expression);
        void emitRegisterWrite(EBPF::CodeBuilder *builder,
                               const IR::MethodCallExpression *expression);
        void emitMethodInvocation(EBPF::CodeBuilder *builder,
                                  const P4::ExternMethod *method);
        void emitKeyInstance(EBPF::CodeBuilder *builder,
                             const IR::MethodCallExpression *expression);
        cstring emitValueInstanceIfNeeded(EBPF::CodeBuilder *builder,
                                          const IR::Argument *arg_value);

    };
}

#endif //P4C_UBPFREGISTER_H