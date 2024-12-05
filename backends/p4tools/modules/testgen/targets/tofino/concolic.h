/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CONCOLIC_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CONCOLIC_H_

#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace P4::P4Tools::P4Testgen::Tofino {

class SharedTofinoConcolic : public Concolic {
 private:
    /// We are not using an enum class because we directly compare integers. This is because error
    /// types are converted into integers in our interpreter. If we use an enum class, we have to
    /// cast every enum access to int.
    struct TofinoHashAlgorithm {
        using Type = enum {
            identity,
            random,
            xor8,
            xor16,
            xor32,
            crc8,
            crc16,
            crc32,
            crc64,
            custom
        };
    };

    /// This represents the chunk size for a hash. Every type bits that is larger than this chunk
    /// size will be split into multiple chunks of size HASH_CHUNK_SIZE;
    static constexpr int HASH_CHUNK_SIZE = 64;

    /// This is the list of concolic functions that are implemented in this class.
    static const ConcolicMethodImpls::ImplList SharedTofinoConcolicMethodImpls;

    /// Helper functions that converts an input list of expressions into a set of literals that have
    /// been evaluated according to the current model. This will be the hash input.
    /// @param resolvedExpressions stores all the variables that have been resolved while converting
    /// the list expression. These variables are later used to create constraints for the SMT
    /// solver that bind a variable to these solutions.
    static const IR::ListExpression *evaluateListHashExpr(
        const std::vector<const IR::Expression *> &exprList, const Model &completedModel,
        Model::ExpressionMap *resolvedExpressions);

 public:
    /// @returns the concolic  functions that are implemented for this particular target.
    static const ConcolicMethodImpls::ImplList *getSharedTofinoConcolicMethodImpls();
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_CONCOLIC_H_ */
