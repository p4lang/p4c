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

/* clang-format off */

#include "bf-p4c/midend/fold_constant_hashes.h"
#include <boost/optional/optional_io.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"
#include "bf-p4c/arch/helpers.h"

namespace BFN {

/**
 * Checks whether the declaration declares a Hash extern.
 * @param[in] decl The declaration to be checked
 * @return Returns the parametrized type if the declaration declares a Hash extern,
 *         nullptr otherwise.
 */
const IR::Type *FoldConstantHashes::checkHashExtern(const IR::Declaration_Instance *decl) {
    if (auto *specialized = decl->type->to<IR::Type_Specialized>()) {
        if (auto *name_type = specialized->baseType->to<IR::Type_Name>()) {
            if (name_type->path->name == "Hash") {
                return specialized->arguments->at(0);
            }
        }
    }
    return nullptr;
}

/**
 * Checks whether the input expression includes only constants. The input expression
 * can include lists, which are checked recursively.
 * @param[in] expr The expression to be checked
 * @return Returns true if the input expression includes only constants.
 */
bool FoldConstantHashes::DoFoldConstantHashes::checkConstantInput(const IR::Expression *expr) {
    if (expr->is<IR::Constant>()) {
        return true;
    } else if (auto *list = expr->to<IR::ListExpression>()) {
        for (auto *comp : list->components) {
            auto *comp_expr = comp->to<IR::Expression>();
            if (!comp_expr || !checkConstantInput(comp_expr)) return false;
        }
        return true;
    } else if (auto *list = expr->to<IR::StructExpression>()) {
        for (auto *comp : list->components) {
            auto *comp_expr = comp->expression->to<IR::Expression>();
            if (!comp_expr || !checkConstantInput(comp_expr)) return false;
        }
        return true;
    }
    return false;
}

/**
 * Recursively folds a list of constants into a single constant of the maximum width of 64 bits.
 * The first element is placed on the highest bits of the result.
 * The elements that overflow are ignored.
 * @param[in,out] value The result
 * @param[in,out] shift The bit position to put the value at
 * @param[in] list The list of expressions used as an input for the hash function
 */
void FoldConstantHashes::DoFoldConstantHashes::foldListToConstant(uint64_t &value, size_t &shift,
        const IR::ListExpression *list) {
    for (auto *comp : boost::adaptors::reverse(list->components)) {
        if (auto *constant = comp->to<IR::Constant>()) {
            value |= constant->asUint64() << shift;
            shift += constant->type->width_bits();
        } else if (auto *list = comp->to<IR::ListExpression>()) {
            foldListToConstant(value, shift, list);
        } else if (auto *list = comp->to<IR::StructExpression>()) {
            foldListToConstant(value, shift, list);
        } else {
            continue;
        }
    }
}
/**
 * Recursively folds a list of constants into a single constant of the maximum width of 64 bits.
 * The first element is placed on the highest bits of the result.
 * The elements that overflow are ignored.
 * @param[in,out] value The result
 * @param[in,out] shift The bit position to put the value at
 * @param[in] list The list of struct expressions used as an input for the hash function
 */
void FoldConstantHashes::DoFoldConstantHashes::foldListToConstant(
    uint64_t &value, size_t &shift, const IR::StructExpression *list) {
    for (auto *comp : boost::adaptors::reverse(list->components)) {
        if (auto *constant = comp->expression->to<IR::Constant>()) {
            value |= constant->asUint64() << shift;
            shift += constant->type->width_bits();
        } else if (auto *list = comp->expression->to<IR::ListExpression>()) {
            foldListToConstant(value, shift, list);
        } else if (auto *list = comp->expression->to<IR::StructExpression>()) {
            foldListToConstant(value, shift, list);
        } else {
            continue;
        }
    }
}

/**
 * Creates a constant expression whose value is constructed from a list of constant expressions.
 * The first element is placed on the highest bits of the result.
 * @param[in] hash_list The list of expressions used as an input for the hash function
 * @param[in] hash_type The type of the newly created constant expression
 * @return Returns newly created constant expression.
 */
const IR::Expression *FoldConstantHashes::DoFoldConstantHashes::substituteIdentityHash(
    const IR::ListExpression *hash_list, const IR::Type *hash_type) {
    uint64_t value = 0;
    size_t shift = 0;
    foldListToConstant(value, shift, hash_list);
    return new IR::Constant(hash_type->clone(), value);
}
const IR::Expression *FoldConstantHashes::DoFoldConstantHashes::substituteIdentityHash(
    const IR::StructExpression *hash_list, const IR::Type *hash_type) {
    uint64_t value = 0;
    size_t shift = 0;
    foldListToConstant(value, shift, hash_list);
    return new IR::Constant(hash_type->clone(), value);
}

/**
 * Computes hash value based on the provided hash fnaction and a list of expressions.
 * @param[in] hash_function The hash function to be used for the computation of the hash value
 * @param[in] hash_list The list of expressions used as an input for the hash function
 * @param[in] hash_type The type of the newly created constant expression
 * @return Returns calculated hash value.
 */
hash_seed_t FoldConstantHashes::DoFoldConstantHashes::computeHash(
    IR::MAU::HashFunction &hash_function, const IR::ListExpression *hash_list,
        const IR::Type *hash_type) {
    bfn_hash_algorithm_t hash_alg;
    hash_function.build_algorithm_t(&hash_alg);

    safe_vector<hash_matrix_output_t> hash_outputs;
    hash_matrix_output_t hash_output;
    hash_output.gfm_start_bit = 0;
    hash_output.p4_hash_output_bit = 0;
    hash_output.bit_size = 0;
    hash_output.bit_size = hash_type->width_bits();
    hash_outputs.push_back(hash_output);

    int total_input_bits = 0;

    safe_vector<ixbar_input_t> hash_inputs;
    for (auto *comp : boost::adaptors::reverse(hash_list->components)) {
        const auto *comp_expr = comp->to<IR::Constant>();
        ixbar_input_t hash_input;
        hash_input.type = ixbar_input_type::tCONST;
        hash_input.u.constant = comp_expr->asUint64();
        hash_input.ixbar_bit_position = 0;
        hash_input.bit_size = comp_expr->type->width_bits();
        hash_input.symmetric_info.is_symmetric = false;
        total_input_bits += hash_input.bit_size;
        hash_inputs.push_back(hash_input);
    }

    hash_seed_t hash_seed{/* hash_seed_value */ 0ULL, /* hash_seed_used */ 0ULL};

    determine_seed(hash_outputs.data(), hash_outputs.size(), hash_inputs.data(), hash_inputs.size(),
                   total_input_bits, &hash_alg, &hash_seed);

    LOG1("  seed = " << std::hex << hash_seed.hash_seed_value << std::dec);
    LOG1("  used = " << std::hex << hash_seed.hash_seed_used << std::dec);

    return hash_seed;
}
/**
 * Computes hash value based on the provided hash fnaction and a list of expressions.
 * @param[in] hash_function The hash function to be used for the computation of the hash value
 * @param[in] hash_list The list of struct expressions used as an input for the hash function
 * @param[in] hash_type The type of the newly created constant expression
 * @return Returns calculated hash value.
 */
hash_seed_t FoldConstantHashes::DoFoldConstantHashes::computeHash(
    IR::MAU::HashFunction &hash_function, const IR::StructExpression *hash_list,
        const IR::Type *hash_type) {
    bfn_hash_algorithm_t hash_alg;
    hash_function.build_algorithm_t(&hash_alg);

    safe_vector<hash_matrix_output_t> hash_outputs;
    hash_matrix_output_t hash_output;
    hash_output.gfm_start_bit = 0;
    hash_output.p4_hash_output_bit = 0;
    hash_output.bit_size = 0;
    hash_output.bit_size = hash_type->width_bits();
    hash_outputs.push_back(hash_output);

    int total_input_bits = 0;

    safe_vector<ixbar_input_t> hash_inputs;
    for (auto *comp : boost::adaptors::reverse(hash_list->components)) {
        const auto *comp_expr = comp->expression->to<IR::Constant>();
        ixbar_input_t hash_input;
        hash_input.type = ixbar_input_type::tCONST;
        hash_input.u.constant = comp_expr->asUint64();
        hash_input.ixbar_bit_position = 0;
        hash_input.bit_size = comp_expr->type->width_bits();
        hash_input.symmetric_info.is_symmetric = false;
        total_input_bits += hash_input.bit_size;
        hash_inputs.push_back(hash_input);
    }

    hash_seed_t hash_seed{/* hash_seed_value */ 0ULL, /* hash_seed_used */ 0ULL};

    determine_seed(hash_outputs.data(), hash_outputs.size(), hash_inputs.data(), hash_inputs.size(),
                   total_input_bits, &hash_alg, &hash_seed);

    LOG1("  seed = " << std::hex << hash_seed.hash_seed_value << std::dec);
    LOG1("  used = " << std::hex << hash_seed.hash_seed_used << std::dec);

    return hash_seed;
}

/**
 * Creates a constant expression whose value is computed by a hash function
 * defined with a CRC polynomial.
 * @param[in] crc_poly_path The path expression representing the CRC polynomial
 * @param[in] hash_list The list of expressions used as an input for the hash function
 * @param[in] hash_type The type of the newly created constant expression
 * @return Returns newly created constant expression.
 */
const IR::Expression *FoldConstantHashes::DoFoldConstantHashes::substituteCustomHash(
    const IR::PathExpression *crc_poly_path, const IR::ListExpression *hash_list,
        const IR::Type *hash_type) {
    auto *crc_poly_decl_inst = getDeclInst(self.refMap, crc_poly_path);
    if (!crc_poly_decl_inst) return nullptr;
    auto *crc_poly_ref = new IR::GlobalRef(crc_poly_decl_inst->srcInfo, crc_poly_decl_inst->type,
                                           crc_poly_decl_inst);
    if (!crc_poly_ref) return nullptr;

    IR::MAU::HashFunction hash_function;
    hash_function.convertPolynomialExtern(crc_poly_ref);

    LOG2("  " << hash_function);

    hash_seed_t hash_seed = computeHash(hash_function, hash_list, hash_type);

    return new IR::Constant(hash_type->clone(), hash_seed.hash_seed_value);
}
/**
 * Creates a constant expression whose value is computed by a hash function
 * defined with a CRC polynomial.
 * @param[in] crc_poly_path The path expression representing the CRC polynomial
 * @param[in] hash_list The list of struct expressions used as an input for the hash function
 * @param[in] hash_type The type of the newly created constant expression
 * @return Returns newly created constant expression.
 */
const IR::Expression *FoldConstantHashes::DoFoldConstantHashes::substituteCustomHash(
    const IR::PathExpression *crc_poly_path, const IR::StructExpression *hash_list,
        const IR::Type *hash_type) {
    auto *crc_poly_decl_inst = getDeclInst(self.refMap, crc_poly_path);
    if (!crc_poly_decl_inst) return nullptr;
    auto *crc_poly_ref = new IR::GlobalRef(crc_poly_decl_inst->srcInfo, crc_poly_decl_inst->type,
                                           crc_poly_decl_inst);
    if (!crc_poly_ref) return nullptr;

    IR::MAU::HashFunction hash_function;
    hash_function.convertPolynomialExtern(crc_poly_ref);

    LOG2("  " << hash_function);

    hash_seed_t hash_seed = computeHash(hash_function, hash_list, hash_type);

    return new IR::Constant(hash_type->clone(), hash_seed.hash_seed_value);
}

/**
 * Creates a constant expression whose value is computed by a pre-defined hash function
 * (e.g. CRCn).
 * @param[in] hash_algo_expr The expression including the type of the hash function to be used
 * @param[in] hash_list The list of expressions used as an input for the hash function
 * @param[in] hash_type The type of the newly created constant expression
 * @return Returns newly created constant expression.
 */
const IR::Expression *FoldConstantHashes::DoFoldConstantHashes::substituteOtherHash(
    const IR::Expression *hash_algo_expr, const IR::ListExpression *hash_list,
        const IR::Type *hash_type) {
    IR::MAU::HashFunction hash_function;
    if (!hash_function.setup(hash_algo_expr)) return nullptr;

    LOG2("  " << hash_function);

    hash_seed_t hash_seed = computeHash(hash_function, hash_list, hash_type);

    return new IR::Constant(hash_type->clone(), hash_seed.hash_seed_value);
}
/**
 * Creates a constant expression whose value is computed by a pre-defined hash function
 * (e.g. CRCn).
 * @param[in] hash_algo_expr The expression including the type of the hash function to be used
 * @param[in] hash_list The list of struct expressions used as an input for the hash function
 * @param[in] hash_type The type of the newly created constant expression
 * @return Returns newly created constant expression.
 */
const IR::Expression *FoldConstantHashes::DoFoldConstantHashes::substituteOtherHash(
    const IR::Expression *hash_algo_expr, const IR::StructExpression *hash_list,
        const IR::Type *hash_type) {
    IR::MAU::HashFunction hash_function;
    if (!hash_function.setup(hash_algo_expr)) return nullptr;

    LOG2("  " << hash_function);

    hash_seed_t hash_seed = computeHash(hash_function, hash_list, hash_type);

    return new IR::Constant(hash_type->clone(), hash_seed.hash_seed_value);
}

/**
 * Substitute hash functions with constant inputs.
 */
const IR::Node *FoldConstantHashes::DoFoldConstantHashes::preorder(IR::MethodCallExpression *mce) {
    auto *method_instance = P4::MethodInstance::resolve(mce, self.refMap, self.typeMap);
    if (!method_instance) return mce;
    auto *extern_object = method_instance->object;
    if (!extern_object) return mce;
    auto *extern_decl = extern_object->to<IR::Declaration_Instance>();
    if (!extern_decl) return mce;

    // Process only invocations of get methods of Hash externs
    auto *hash_type = self.checkHashExtern(extern_decl);
    if (!hash_type || method_instance->actualMethodType->toString() != "get") return mce;

    auto hash_algo_expr = extern_decl->arguments->at(0)->expression;
    auto hash_algo_name = extern_decl->arguments->at(0)->toString();
    auto hash_expr = mce->arguments->at(0)->expression;

    // Process only invocations with constant inputs
    if (!checkConstantInput(hash_expr)) return mce;

    // Input values are represented as a list expression, even if it is a single value
    auto *hash_list = hash_expr->to<IR::StructExpression>();
    // Convert single value into a list
    if (hash_expr->is<IR::Constant>()) {
        IR::IndexedVector<IR::NamedExpression> components;
        components.push_back(new IR::NamedExpression(IR::ID("temp_const"), hash_expr));
        hash_list = new IR::StructExpression(hash_expr->srcInfo, nullptr, components);
    }
    // Convert possible ListExpression
    if (hash_expr->is<IR::ListExpression>()) {
        IR::IndexedVector<IR::NamedExpression> components;
        int i = 0;
        for (const auto &comp : hash_expr->to<IR::ListExpression>()->components) {
            components.push_back(
                new IR::NamedExpression(IR::ID("temp_name" + cstring::to_cstring(i++)), comp));
        }
        hash_list = new IR::StructExpression(hash_expr->srcInfo, nullptr, components);
    }
    CHECK_NULL(hash_list);

    // Newly created constant expression
    const IR::Expression *rv = mce;

    if (hash_algo_name == "HashAlgorithm_t.RANDOM") {
        LOG1("Skipping random hash " << method_instance->expr);
        return mce;
    } else if (hash_algo_name == "HashAlgorithm_t.IDENTITY") {
        LOG1("Substituting identity hash " << method_instance->expr);
        rv = substituteIdentityHash(hash_list, hash_type);
    } else if (hash_algo_name == "HashAlgorithm_t.CUSTOM") {
        LOG1("Substituting custom hash " << method_instance->expr);
        auto *crc_poly_path = extern_decl->arguments->at(1)->expression->to<IR::PathExpression>();
        if (!crc_poly_path) return mce;
        rv = substituteCustomHash(crc_poly_path, hash_list, hash_type);
        auto *crc_poly_decl_inst = getDeclInst(self.refMap, crc_poly_path);
        if (!crc_poly_decl_inst) return mce;
        // Mark the CRCPolynomial extern object for removal
        self.candidatesToRemove.insert(crc_poly_decl_inst->declid);
    } else {
        LOG1("Substituting other hash " << method_instance->expr);
        rv = substituteOtherHash(hash_algo_expr, hash_list, hash_type);
    }

    // Mark the Hash extern object as a candidate for removal
    self.candidatesToRemove.insert(extern_decl->declid);

    // Mark the invocations whose result is not used to be removed.
    // Do not apply the substitution since it would produce a method
    // call statement without a method call expression.
    if (auto *mcs = getParent<IR::MethodCallStatement>()) {
        self.methodCallStatementsToRemove.insert(mcs->clone_id);
        return mce;
    }

    return rv;
}

/**
 * Do not remove declarations of candidates that are used in path expressions.
 */
const IR::Node *FoldConstantHashes::CheckCandidesToRemove::preorder(IR::PathExpression *path) {
    auto *decl_inst = getDeclInst(self.refMap, path);
    if (!decl_inst) return path;
    auto *mcs = findContext<IR::MethodCallStatement>();
    if (!mcs) return path;
    // The declaration of candidates that are used in method call statements,
    // which means their result is not used, are to be removed.
    if (self.methodCallStatementsToRemove.count(mcs->clone_id) == 0)
        self.candidatesToRemove.erase(decl_inst->declid);
    return path;
}

/**
 * Remove declarations of candidates that have not been removed in the CheckCandidesToRemove pass.
 */
const IR::Node *FoldConstantHashes::RemoveHangingCandidates::preorder(
        IR::Declaration_Instance *decl) {
    if (self.candidatesToRemove.count(decl->declid) == 0) return decl;
    LOG2("Removing " << decl);
    return nullptr;
}

/**
 * Remove the invocations whose result is not used.
 * It would cause a compiler bug when replaced with a constant.
 */
const IR::Node *FoldConstantHashes::RemoveHangingCandidates::preorder(
        IR::MethodCallStatement *mcs) {
    if (self.methodCallStatementsToRemove.count(mcs->clone_id) == 0) return mcs;
    LOG2("Removing " << mcs);
    return nullptr;
}

}  // namespace BFN
