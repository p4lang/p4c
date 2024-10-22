/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_STATEFUL_ALU_H_
#define BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_STATEFUL_ALU_H_

#include "frontends/p4-14/fromv1.0/converters.h"

namespace P4 {
namespace P4V1 {

class StatefulAluConverter : public ExternConverter {
    bool has_stateful_alu = false;
    struct reg_info {
        const IR::Register *reg = nullptr;
        const IR::Type::Bits *utype = nullptr;  // salu alu type
        const IR::Type *rtype = nullptr;        // layout type
    };
    std::map<const IR::Register *, reg_info> cache;
    const IR::ActionProfile *getSelectorProfile(P4V1::ProgramStructure *,
                                                const IR::Declaration_Instance *);
    reg_info getRegInfo(P4V1::ProgramStructure *, const IR::Declaration_Instance *,
                        IR::Vector<IR::Node> *);
    const IR::Type::Bits *findUType(const IR::Declaration_Instance *, const IR::Type ** = nullptr);
    StatefulAluConverter();
    static StatefulAluConverter singleton;

 public:
    const IR::Type_Extern *convertExternType(P4V1::ProgramStructure *, const IR::Type_Extern *,
                                             cstring) override;
    const IR::Declaration_Instance *convertExternInstance(
        P4V1::ProgramStructure *, const IR::Declaration_Instance *, cstring,
        IR::IndexedVector<IR::Declaration> *) override;
    const IR::Statement *convertExternCall(P4V1::ProgramStructure *,
                                           const IR::Declaration_Instance *,
                                           const IR::Primitive *) override;
};

}  // namespace P4V1
}  // namespace P4

#endif /* BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_STATEFUL_ALU_H_ */
