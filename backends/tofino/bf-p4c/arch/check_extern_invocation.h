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

#ifndef BF_P4C_ARCH_CHECK_EXTERN_INVOCATION_H_
#define BF_P4C_ARCH_CHECK_EXTERN_INVOCATION_H_

#include "bf-p4c/arch/arch.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "midend/checkExternInvocationCommon.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace BFN {

class CheckExternInvocationCommon : public P4::CheckExternInvocationCommon {
 protected:
    int genIndex(gress_t gress, ArchBlock_t block) {
        return gress * ArchBlock_t::BLOCK_TYPE + block;
    }
    cstring getBlockName(int bit) override {
        static const char *lookup[] = {"parser", "control (MAU)", "deparser"};
        BUG_CHECK(sizeof(lookup) / sizeof(lookup[0]) == ArchBlock_t::BLOCK_TYPE,
                  "Bad lookup table");
        return cstring(lookup[bit % ArchBlock_t::BLOCK_TYPE]);
    }
    void initCommonPipeConstraints();
    void checkExtern(const P4::ExternMethod *extMethod,
                     const IR::MethodCallExpression *expr) override;
    CheckExternInvocationCommon(P4::ReferenceMap *, P4::TypeMap *typeMap)
        : P4::CheckExternInvocationCommon(typeMap) {}
};

class CheckTNAExternInvocation : public CheckExternInvocationCommon {
    void initPipeConstraints() override;

 public:
    CheckTNAExternInvocation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : CheckExternInvocationCommon(refMap, typeMap) {
        initPipeConstraints();
    }
};

class CheckT2NAExternInvocation : public CheckExternInvocationCommon {
    void initPipeConstraints() override;

 public:
    CheckT2NAExternInvocation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : CheckExternInvocationCommon(refMap, typeMap) {
        initPipeConstraints();
    }
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_CHECK_EXTERN_INVOCATION_H_ */
