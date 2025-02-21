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

#include "backends/p4tools/modules/testgen/targets/tofino/hash_utils.h"

#include <ostream>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "backends/tofino/bf-utils/dynamic_hash/bfn_hash_algorithm.h"
#include "backends/tofino/bf-utils/dynamic_hash/dynamic_hash.h"
#include "ir/irutils.h"
#include "lib/log.h"
#include "lib/safe_vector.h"

namespace P4::P4Tools::P4Testgen::Tofino {

hash_seed_t HashCompute::computeHash(IR::MAU::HashFunction &hashFunction,
                                     const IR::ListExpression *hashList, const IR::Type *hashType,
                                     const std::vector<bool> * /*symmetricList*/) {
    bfn_hash_algorithm_t hashAlg;
    hashFunction.build_algorithm_t(&hashAlg);

    safe_vector<hash_matrix_output_t> hashOutputs;
    hash_matrix_output_t hashOutput;
    hashOutput.gfm_start_bit = 0;
    hashOutput.p4_hash_output_bit = 0;
    hashOutput.bit_size = 0;
    hashOutput.bit_size = hashType->width_bits();
    hashOutputs.push_back(hashOutput);

    int totalInputBits = 0;

    safe_vector<ixbar_input_t> hashInputs;
    for (const auto *comp : boost::adaptors::reverse(hashList->components)) {
        const auto *compExpr = comp->to<IR::Constant>();
        ixbar_input_t hashInput;
        hashInput.type = ixbar_input_type::tCONST;
        hashInput.u.constant = compExpr->asUint64();
        hashInput.ixbar_bit_position = 0;
        hashInput.bit_size = compExpr->type->width_bits();
        hashInput.symmetric_info.is_symmetric = false;
        totalInputBits += hashInput.bit_size;
        hashInputs.push_back(hashInput);
    }

    hash_seed_t hashSeed = {0ULL, 0ULL};

    determine_seed(hashOutputs.data(), hashOutputs.size(), hashInputs.data(), hashInputs.size(),
                   totalInputBits, &hashAlg, &hashSeed);

    LOG1("  seed = " << std::hex << hashSeed.hash_seed_value << std::dec);
    LOG1("  used = " << std::hex << hashSeed.hash_seed_used << std::dec);

    return hashSeed;
}

const IR::Constant *HashCompute::substituteCustomHash(const IR::Declaration_Instance *polyInstance,
                                                      const IR::ListExpression *hashList,
                                                      const IR::Type *hashType,
                                                      const std::vector<bool> *symmetricList) {
    auto *crcPolyRef = new IR::GlobalRef(polyInstance->srcInfo, polyInstance->type, polyInstance);
    if (crcPolyRef == nullptr) {
        return nullptr;
    }

    IR::MAU::HashFunction hashFunction;
    hashFunction.convertPolynomialExtern(crcPolyRef);

    // LOG2("  " << hashFunction);

    hash_seed_t hashSeed = computeHash(hashFunction, hashList, hashType, symmetricList);

    return IR::Constant::get(hashType->clone(), hashSeed.hash_seed_value);
}

const IR::Constant *HashCompute::substituteOtherHash(const IR::Expression *hashAlgoExpr,
                                                     const IR::ListExpression *hashList,
                                                     const IR::Type *hashType,
                                                     const std::vector<bool> *symmetricList) {
    IR::MAU::HashFunction hashFunction;
    if (!hashFunction.setup(hashAlgoExpr)) {
        return nullptr;
    }

    // LOG2("  " << hashFunction);

    hash_seed_t hashSeed = computeHash(hashFunction, hashList, hashType, symmetricList);

    return IR::Constant::get(hashType->clone(), hashSeed.hash_seed_value);
}

}  // namespace P4::P4Tools::P4Testgen::Tofino
