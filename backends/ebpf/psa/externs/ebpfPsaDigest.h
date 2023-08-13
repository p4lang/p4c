/*
Copyright 2022-present Orange

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

#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSADIGEST_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSADIGEST_H_

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfObject.h"
#include "backends/ebpf/ebpfProgram.h"
#include "backends/ebpf/ebpfType.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace EBPF {

class DeparserBodyTranslatorPSA;

class EBPFDigestPSA : public EBPFObject {
 private:
    cstring instanceName;
    const EBPFProgram *program;
    EBPFType *valueType;
    cstring valueTypeName;
    const IR::Declaration_Instance *declaration;
    // arbitrary value for max queue size
    // TODO: make it configurable
    int maxDigestQueueSize = 128;

 public:
    EBPFDigestPSA(const EBPFProgram *program, const IR::Declaration_Instance *di);

    void emitTypes(CodeBuilder *builder);
    void emitInstance(CodeBuilder *builder) const;
    void processMethod(CodeBuilder *builder, cstring method, const IR::MethodCallExpression *expr,
                       DeparserBodyTranslatorPSA *visitor);

    void emitPushElement(CodeBuilder *builder, const IR::Expression *elem,
                         Inspector *codegen) const;
    void emitPushElement(CodeBuilder *builder, cstring elem) const;
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EXTERNS_EBPFPSADIGEST_H_ */
