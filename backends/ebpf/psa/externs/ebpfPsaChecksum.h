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
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACHECKSUM_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACHECKSUM_H_

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfObject.h"
#include "ebpfPsaHashAlgorithm.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace EBPF {

class EBPFChecksumPSA : public EBPFObject {
 protected:
    EBPFHashAlgorithmPSA *engine;
    const IR::Declaration_Instance *declaration;

    void init(const EBPFProgram *program, cstring name, int type);

 public:
    EBPFChecksumPSA(const EBPFProgram *program, const IR::Declaration_Instance *block,
                    cstring name);

    EBPFChecksumPSA(const EBPFProgram *program, const IR::Declaration_Instance *block, cstring name,
                    int type);

    void emitVariables(CodeBuilder *builder) {
        if (engine) engine->emitVariables(builder, declaration);
    }

    virtual void processMethod(CodeBuilder *builder, cstring method,
                               const IR::MethodCallExpression *expr, Visitor *visitor);
};

class EBPFInternetChecksumPSA : public EBPFChecksumPSA {
 public:
    EBPFInternetChecksumPSA(const EBPFProgram *program, const IR::Declaration_Instance *block,
                            cstring name)
        : EBPFChecksumPSA(program, block, name,
                          EBPFHashAlgorithmPSA::HashAlgorithm::ONES_COMPLEMENT16) {}

    void processMethod(CodeBuilder *builder, cstring method, const IR::MethodCallExpression *expr,
                       Visitor *visitor) override;
};

class EBPFHashPSA : public EBPFChecksumPSA {
 public:
    EBPFHashPSA(const EBPFProgram *program, const IR::Declaration_Instance *block, cstring name)
        : EBPFChecksumPSA(program, block, name) {}

    void processMethod(CodeBuilder *builder, cstring method, const IR::MethodCallExpression *expr,
                       Visitor *visitor) override;

    void calculateHash(CodeBuilder *builder, const IR::MethodCallExpression *expr,
                       Visitor *visitor);

    void emitGetMethod(CodeBuilder *builder, const IR::MethodCallExpression *expr,
                       Visitor *visitor);
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACHECKSUM_H_ */
