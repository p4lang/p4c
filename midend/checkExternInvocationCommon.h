/*
Copyright 2021 Intel Corp.

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

#ifndef MIDEND_CHECKEXTERNINVOCATIONCOMMON_H_
#define MIDEND_CHECKEXTERNINVOCATIONCOMMON_H_

#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/bitvec.h"

namespace P4 {

class ReferenceMap;
class TypeMap;

/**
 * @brief Base class which can be used to prepare classes for checking constraints
 *        for invocations of externs (both methods and pure functions) in parsers and
 *        control blocks.
 *
 * This class contains basic operations which should be common for checkers used for
 * various targets and architectures.
 *
 * Example class which inherits from this base class can be seen
 * in backends/dpdk/dpdkCheckExternInvocation.h.
 */
class CheckExternInvocationCommon : public Inspector {
 protected:
    ReferenceMap *refMap;
    TypeMap *typeMap;
    std::map<cstring /* extType */, bitvec> pipeConstraints;

    /**
     * @brief Method used to initialize the constraints.
     *
     * Method setPipeConstraints() can be used in implementation of initPipeConstraints()
     * method to initialize the constraints.
     *
     * Method initPipeConstraints() should be called in the constructor of inheriting class.
     */
    virtual void initPipeConstraints() = 0;

    /**
     * @brief Get the name of the block which is represented in bit vector (bitvec)
     *        by bit with index given by 'bit' parameter.
     *
     * @param bit Index of bit which represents the block in bit vector.
     * @return cstring Name of the block represented by index with 'bit' parameter value.
     */
    virtual cstring getBlockName(int bit) = 0;

    /**
     * @brief Method for checking constraints of extern method given by parameters.
     *
     * If there are no constraints for extern methods, inheriting class does not need
     * to implement this method.
     *
     * @param extMethod Pointer to object representing extern method.
     * @param expr Pointer to method call expression.
     */
    virtual void checkExtern(const ExternMethod *extMethod, const IR::MethodCallExpression *expr) {
        (void)extMethod;
        (void)expr;
    }

    /**
     * @brief Method for checking constraints of extern functions given by parameters.
     *
     * If there are no constraints for extern functions, inheriting class does not need
     * to implement this method.
     *
     * @param extMethod Pointer to object representing extern function.
     * @param expr Pointer to function call expression.
     */
    virtual void checkExtern(const ExternFunction *extFunction,
                             const IR::MethodCallExpression *expr) {
        (void)extFunction;
        (void)expr;
    }

    /**
     * @brief Get the name of the block which is represented by bit set in the bitvec.
     *
     * @param vec Bit vector.
     * @return cstring Name of the block represented by index of set bit in the bitvec.
     */
    cstring extractBlock(bitvec vec) {
        int bit = vec.ffs(0);
        BUG_CHECK(vec.ffs(bit), "Trying to extract multiple block encodings");
        return getBlockName(bit);
    }

    /**
     * @brief Set the pipe (parser/control block) constraints.
     *
     * Should be used in the initPipeConstraints() method.
     *
     * @param extType Name of the extern object or function for which we set constraints.
     * @param vec Bit vector representing the blocks (parser/control) in which the use
     *            of extern object method or function is valid.
     */
    void setPipeConstraints(cstring extType, bitvec vec) {
        if (!pipeConstraints.count(extType)) {
            pipeConstraints.emplace(extType, vec);
        } else {
            auto &cons = pipeConstraints.at(extType);
            cons |= vec;
        }
    }

    /**
     * @brief Check if the invocation of extern object method or extern function is valid
     *        in the block where it is invoked.
     *
     * @param extType Name of the extern object or extern function.
     * @param bv Bit vector which has set the bit representing the block in which the extern
     *           object method or extern function is invoked.
     * @param expr Method or function call expression.
     * @param extName Name of extern object in case of extern object method invocation.
     *                Empty string ("") in case of extern function invocation.
     * @param pipe Name of the parser or control block in which the method or the function
     *             is invoked.
     */
    void checkPipeConstraints(cstring extType, bitvec bv, const IR::MethodCallExpression *expr,
                              cstring extName, cstring pipe) {
        BUG_CHECK(pipeConstraints.count(extType), "pipe constraints not defined for %1%", extType);
        auto constraint = pipeConstraints.at(extType) & bv;
        if (!bv.empty() && constraint.empty()) {
            if (extName != "")
                ::error(ErrorType::ERR_UNSUPPORTED, "%s %s %s cannot be used in the %s %s",
                        expr->srcInfo, extType, extName, pipe, extractBlock(bv));
            else
                ::error(ErrorType::ERR_UNSUPPORTED, "%s %s cannot be used in the %s %s",
                        expr->srcInfo, extType, pipe, extractBlock(bv));
        }
    }

    CheckExternInvocationCommon(ReferenceMap *refMap, TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}

 public:
    bool preorder(const IR::MethodCallExpression *expr) override {
        auto mi = MethodInstance::resolve(expr, refMap, typeMap);
        if (auto extMethod = mi->to<ExternMethod>()) {
            checkExtern(extMethod, expr);
        } else if (auto extFunction = mi->to<ExternFunction>()) {
            checkExtern(extFunction, expr);
        }
        return false;
    }
};

}  // namespace P4

#endif /* MIDEND_CHECKEXTERNINVOCATIONCOMMON_H_ */
