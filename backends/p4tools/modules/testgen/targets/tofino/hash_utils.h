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

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_HASH_UTILS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_HASH_UTILS_H_

#include <vector>

#include "backends/tofino/bf-utils/include/dynamic_hash/dynamic_hash.h"
#include "ir/ir.h"
// Need to include this last.
#include "backends/tofino//bf-p4c/mau/hash_function.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/** This class defines helper functions that can be used to calculate Tofino hashes from a list of
 * supplied constants. This code is lifted from the FoldConstantHashes in "fold_constant_hashes.h"
 * of the bf-p4c midend.
 */
class HashCompute {
 private:
    /**
     * Computes hash value based on the provided hash function and a list of expressions.
     * @param[in] hashFunction The hash function to be used for the computation of the hash value
     * @param[in] hashList The list of expressions used as an input for the hash function
     * @param[in] hashType The type of the newly created constant expression
     * @param[in] symmetricList A map indicating which values of the @param hashList are symmetric.
     * @return Returns calculated hash value.
     */
    static hash_seed_t computeHash(IR::MAU::HashFunction &hashFunction,
                                   const IR::ListExpression *hashList, const IR::Type *hashType,
                                   const std::vector<bool> *symmetricList);

 public:
    /**
     * Creates a constant expression whose value is computed by a hash function
     * defined with a CRC polynomial.
     * @param[in] crc_poly_path The path expression representing the CRC polynomial
     * @param[in] hashList The list of expressions used as an input for the hash function
     * @param[in] hashType The type of the newly created constant expression
     * @param[in] symmetricList A map indicating which values of the @param hashList are symmetric.
     * @return Returns newly created constant expression.
     */
    static const IR::Constant *substituteCustomHash(const IR::Declaration_Instance *polyInstance,
                                                    const IR::ListExpression *hashList,
                                                    const IR::Type *hashType,
                                                    const std::vector<bool> *symmetricList);
    /**
     * Creates a constant expression whose value is computed by a pre-defined hash function
     * (e.g. CRCn).
     * @param[in] hashAlgoExpr The expression including the type of the hash function to be used
     * @param[in] hashList The list of expressions used as an input for the hash function
     * @param[in] hashType The type of the newly created constant expression
     * @param[in] symmetricList A map indicating which values of the @param hashList are symmetric.
     * @return Returns newly created constant expression.
     */
    static const IR::Constant *substituteOtherHash(const IR::Expression *hashAlgoExpr,
                                                   const IR::ListExpression *hashList,
                                                   const IR::Type *hashType,
                                                   const std::vector<bool> *symmetricList);
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_HASH_UTILS_H_ */
