/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_ARCH_CHECK_EXTERN_INVOCATION_H_
#define BF_P4C_ARCH_CHECK_EXTERN_INVOCATION_H_

#include "ir/ir.h"
#include "ir/visitor.h"
#include "bf-p4c/arch/arch.h"
#include "frontends/p4/methodInstance.h"
#include "midend/checkExternInvocationCommon.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}

namespace BFN {

class CheckExternInvocationCommon : public P4::CheckExternInvocationCommon {
 protected:
    int genIndex(gress_t gress, ArchBlock_t block) {
        return gress * ArchBlock_t::BLOCK_TYPE + block;
    }
    cstring getBlockName(int bit) override {
        static const char* lookup[] = {"parser", "control (MAU)", "deparser"};
        BUG_CHECK(sizeof(lookup)/sizeof(lookup[0]) == ArchBlock_t::BLOCK_TYPE, "Bad lookup table");
        return cstring(lookup[bit % ArchBlock_t::BLOCK_TYPE]);
    }
    void initCommonPipeConstraints();
    void checkExtern(const P4::ExternMethod *extMethod,
            const IR::MethodCallExpression *expr) override;
    CheckExternInvocationCommon(P4::ReferenceMap *, P4::TypeMap *typeMap) :
        P4::CheckExternInvocationCommon(typeMap) {}
};

class CheckTNAExternInvocation : public CheckExternInvocationCommon {
    void initPipeConstraints() override;
 public:
    CheckTNAExternInvocation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) :
        CheckExternInvocationCommon(refMap, typeMap) { initPipeConstraints(); }
};

class CheckT2NAExternInvocation : public CheckExternInvocationCommon {
    void initPipeConstraints() override;
 public:
    CheckT2NAExternInvocation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) :
        CheckExternInvocationCommon(refMap, typeMap) { initPipeConstraints(); }
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_CHECK_EXTERN_INVOCATION_H_ */
