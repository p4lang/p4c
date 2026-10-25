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

#include "backends/p4tools/modules/testgen/targets/tofino/concolic.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/null.h"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/tofino/hash_utils.h"

namespace P4::P4Tools::P4Testgen::Tofino {

const IR::ListExpression *SharedTofinoConcolic::evaluateListHashExpr(
    const std::vector<const IR::Expression *> &exprList, const Model &completedModel,
    Model::ExpressionMap *resolvedExpressions) {
    auto *evaluatedListExpr = new IR::ListExpression({});
    for (const auto *expr : exprList) {
        auto width = expr->type->width_bits();
        if (width > HASH_CHUNK_SIZE) {
            auto chunks = width / HASH_CHUNK_SIZE;
            auto remainder = width % HASH_CHUNK_SIZE;
            for (int i = 0; i < chunks; ++i) {
                auto hi = width - i * HASH_CHUNK_SIZE - 1;
                auto lo = hi - HASH_CHUNK_SIZE + 1;
                auto *slice = new IR::Slice(expr, hi, lo);
                slice->type = IR::Type_Bits::get(HASH_CHUNK_SIZE);
                const auto *evalConst = completedModel.evaluate(slice, true, resolvedExpressions);
                evaluatedListExpr->components.push_back(evalConst);
            }
            if (remainder != 0) {
                auto hi = width - chunks * HASH_CHUNK_SIZE - 1;
                auto lo = 0;
                auto *slice = new IR::Slice(expr, hi, lo);
                slice->type = IR::Type_Bits::get(remainder);
                const auto *evalConst = completedModel.evaluate(slice, true, resolvedExpressions);
                evaluatedListExpr->components.push_back(evalConst);
            }
        } else {
            const auto *evalConst = completedModel.evaluate(expr, true, resolvedExpressions);
            evaluatedListExpr->components.push_back(evalConst);
        }
    }
    return evaluatedListExpr;
}

const ConcolicMethodImpls::ImplList SharedTofinoConcolic::SharedTofinoConcolicMethodImpls{
    /* ======================================================================================
     *   Hash_get
     * ====================================================================================== */

    {"Hash_get"_cs,
     {"data"_cs},
     [](cstring /*qualifiedMethodName*/, const IR::ConcolicVariable *var,
        const ExecutionState & /*state*/, const Model &completedModel,
        ConcolicVariableMap *resolvedConcolicVariables) {
         const auto *returnType = var->type;

         auto externDecls = var->associatedNodes;
         BUG_CHECK(!externDecls.empty(),
                   "Must have at least 1 declaration associated with this extern.");
         const auto *externInstance = externDecls.at(0)->checkedTo<IR::Declaration_Instance>();

         const auto *externInstanceArgs = externInstance->arguments;
         BUG_CHECK(externInstanceArgs->size() == 1 || externInstanceArgs->size() == 2,
                   "Hash types either have one instance argument or two.");
         CHECK_NULL(externInstanceArgs->at(0)->expression);
         const auto *symmetricAnno = externInstance->getAnnotation("symmetric"_cs);
         std::vector<bool> symmetricList;
         if (symmetricAnno != nullptr) {
             ::warning("Symmetric annotations for hashes are currently not implemented.");
         }

         const auto *args = var->arguments;
         const auto *dataExpr = args->at(0)->expression;
         std::vector<const IR::Expression *> exprList;
         // We only support struct expressions as argument input for now.
         if (const auto *dataStructExpr = dataExpr->to<IR::StructExpression>()) {
             exprList = IR::flattenStructExpression(dataStructExpr);
         } else if (dataExpr->is<IR::Literal>() || dataExpr->is<IR::SymbolicVariable>()) {
             exprList.emplace_back(dataExpr);
         } else {
             TESTGEN_UNIMPLEMENTED("Hash input %1% of type %2% is not supported.", dataExpr,
                                   dataExpr->node_type_name());
         }
         if (exprList.empty()) {
             TESTGEN_UNIMPLEMENTED("Data input is empty. This case is not implemented.");
         }

         // All the preprocessing is completed. Now pick the hash algorithm according to the
         // argument given to the extern declaration.
         const IR::Expression *computedResult = nullptr;
         const IR::ListExpression *evaluatedListExpr = nullptr;
         Model::ExpressionMap resolvedExpressions;
         const auto *hashAlgoExpr = externInstance->arguments->at(0)->expression;
         // In our case, enums have been converted to concrete values.
         // TODO: Maybe look up the declaration?
         auto hashAlgoIdx = hashAlgoExpr->checkedTo<IR::Constant>()->asInt();

         if (hashAlgoIdx == TofinoHashAlgorithm::identity) {
             const auto *concatExpr = exprList.at(0);
             for (size_t idx = 1; idx < exprList.size(); idx++) {
                 const auto *expr = exprList.at(idx);
                 const auto *newWidth =
                     IR::Type_Bits::get(concatExpr->type->width_bits() + expr->type->width_bits());
                 concatExpr = new IR::Concat(newWidth, concatExpr, expr);
             }
             computedResult =
                 IR::Constant::get(returnType, IR::getBigIntFromLiteral(completedModel.evaluate(
                                                   concatExpr, true, &resolvedExpressions)));
         } else if (hashAlgoIdx == TofinoHashAlgorithm::random) {
             BUG("Random hashes should not be encountered here.");
         } else if (hashAlgoIdx == TofinoHashAlgorithm::custom) {
             // For custom algorithms we need the CRCDeclaration. Try to find it as the second
             // argument in the supplied extern declarations.
             const IR::Declaration_Instance *crcDeclInstance = nullptr;
             BUG_CHECK(externDecls.size() >= 2,
                       "The number of extern declarations supplied to this concolic call must at "
                       "least be 2.");
             crcDeclInstance = externDecls.at(1)->checkedTo<IR::Declaration_Instance>();
             evaluatedListExpr =
                 evaluateListHashExpr(exprList, completedModel, &resolvedExpressions);
             computedResult = HashCompute::substituteCustomHash(crcDeclInstance, evaluatedListExpr,
                                                                returnType, &symmetricList);
         } else {
             evaluatedListExpr =
                 evaluateListHashExpr(exprList, completedModel, &resolvedExpressions);
             computedResult = HashCompute::substituteOtherHash(hashAlgoExpr, evaluatedListExpr,
                                                               returnType, &symmetricList);
         }

         // Assign a constraint for the concolic variable using the computed result.
         if (returnType->is<IR::Type_Bits>()) {
             // Overwrite any previous assignment or result.
             (*resolvedConcolicVariables)[*var] = computedResult;
         } else {
             TESTGEN_UNIMPLEMENTED("Hash return type %2% not supported", returnType);
         }
         // Generated equations for all the variables that were assigned a value in this iteration
         // of concolic execution.
         for (const auto &variable : resolvedExpressions) {
             const auto *varName = variable.first;
             const auto *varExpr = variable.second;
             (*resolvedConcolicVariables)[varName] = varExpr;
         }
     }},
};

const ConcolicMethodImpls::ImplList *SharedTofinoConcolic::getSharedTofinoConcolicMethodImpls() {
    return &SharedTofinoConcolicMethodImpls;
}

}  // namespace P4::P4Tools::P4Testgen::Tofino
