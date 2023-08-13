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
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSARANDOM_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSARANDOM_H_

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfObject.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"

namespace EBPF {

class EBPFRandomPSA : public EBPFObject {
    unsigned int minValue, maxValue;
    long range;

 public:
    explicit EBPFRandomPSA(const IR::Declaration_Instance *di);

    void processMethod(CodeBuilder *builder, const P4::ExternMethod *method) const;
    void emitRead(CodeBuilder *builder) const;
};

}  // namespace EBPF

#endif  // BACKENDS_EBPF_PSA_EXTERNS_EBPFPSARANDOM_H_
