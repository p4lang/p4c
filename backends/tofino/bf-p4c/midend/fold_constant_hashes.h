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

#ifndef BF_P4C_MIDEND_FOLD_CONSTANT_HASHES_H_
#define BF_P4C_MIDEND_FOLD_CONSTANT_HASHES_H_

/* clang-format off */
#include "ir/ir.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf-p4c/mau/hash_function.h"
#include "bf-utils/include/dynamic_hash/dynamic_hash.h"

namespace BFN {

/**
 * \ingroup midend
 * \brief PassManager that substitutes the calls of the get methods of the Hash externs
 *        whose inputs are constants with the resulting hash value.
 * 
 * The input can be a constant value or a list of constant values.
 * The width of the resulting value is determined by the Hash extern.
 *
 * E.g.:
 * 
 *     Hash<bit<16>>(HashAlgorithm_t.IDENTITY) hash;
 *     value = hash.get<tuple<bit<8>, bit<8>, bit<8>, bit<8>>>({ 1, 2, 3, 4 });
 * 
 * is replaced with:
 * 
 *     value = 772;  // which is 0x0304
 * 
 * The original object of the Hash extern is removed.
 *
 * In the case of the identity hash, if a list of constant values is used,
 * the first element is placed on the highest bits of the result.
 * The elements that overflow are ignored.
 * 
 * The Hash extern objects on which the substitution has taken place
 * are checked and if not used elsewhere, their declaration is remove.
 * The same applies to the CRCPolynomial extern objects used in the
 * Hash extern objects of the custom type.
 */
class FoldConstantHashes : public PassManager {
    P4::ReferenceMap *refMap = nullptr;
    P4::TypeMap *typeMap = nullptr;

    // The set of declid's of the Hash or CRCPolynomial objects that are candidates for removal
    std::set<int> candidatesToRemove;
    // The set of method call statements to be removed
    std::set<int> methodCallStatementsToRemove;

    class DoFoldConstantHashes : public Transform {
        FoldConstantHashes &self;

        bool checkConstantInput(const IR::Expression *expr);
        void foldListToConstant(uint64_t &value, size_t &shift, const IR::ListExpression *list);
        void foldListToConstant(uint64_t &value, size_t &shift, const IR::StructExpression *list);
        const IR::Expression *substituteIdentityHash(const IR::ListExpression *hash_list,
                const IR::Type *hash_type);
        const IR::Expression *substituteIdentityHash(const IR::StructExpression *hash_list,
                const IR::Type *hash_type);

        hash_seed_t computeHash(IR::MAU::HashFunction &hash_function,
                                const IR::ListExpression *hash_list, const IR::Type *hash_type);
        hash_seed_t computeHash(IR::MAU::HashFunction &hash_function,
                                const IR::StructExpression *hash_list, const IR::Type *hash_type);
        const IR::Expression *substituteCustomHash(const IR::PathExpression *crc_poly_path,
                const IR::ListExpression *hash_list,
                const IR::Type *hash_type);
        const IR::Expression *substituteCustomHash(const IR::PathExpression *crc_poly_path,
                const IR::StructExpression *hash_list,
                const IR::Type *hash_type);
        const IR::Expression *substituteOtherHash(const IR::Expression *hash_algo_expr,
                const IR::ListExpression *hash_list,
                const IR::Type *hash_type);
        const IR::Expression *substituteOtherHash(const IR::Expression *hash_algo_expr,
                const IR::StructExpression *hash_list,
                const IR::Type *hash_type);

     public:
        explicit DoFoldConstantHashes(FoldConstantHashes &self) : self(self) {}
        const IR::Node *preorder(IR::MethodCallExpression *mce) override;
    };

    class CheckCandidesToRemove : public Transform {
        FoldConstantHashes &self;

     public:
        explicit CheckCandidesToRemove(FoldConstantHashes &self) : self(self) {}
        const IR::Node *preorder(IR::PathExpression *path) override;
    };

    class RemoveHangingCandidates : public Transform {
        FoldConstantHashes &self;

     public:
        explicit RemoveHangingCandidates(FoldConstantHashes &self) : self(self) {}
        const IR::Node *preorder(IR::Declaration_Instance *decl) override;
        const IR::Node *preorder(IR::MethodCallStatement *mcs) override;
    };

    const IR::Type *checkHashExtern(const IR::Declaration_Instance *decl);

 public:
    FoldConstantHashes(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                       BFN::TypeChecking *typeChecking = nullptr)
        : refMap(refMap), typeMap(typeMap) {
        if (!typeChecking) typeChecking = new BFN::TypeChecking(refMap, typeMap);
        addPasses({typeChecking, new DoFoldConstantHashes(*this), new P4::ClearTypeMap(typeMap),
                   typeChecking, new CheckCandidesToRemove(*this),
                   new RemoveHangingCandidates(*this), new P4::ClearTypeMap(typeMap)});
    }
};

}  // namespace BFN

/* clang-format on */

#endif  // BF_P4C_MIDEND_FOLD_CONSTANT_HASHES_H_
