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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_HASH_MASK_ANNOTATIONS_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_HASH_MASK_ANNOTATIONS_H_

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"
#include "lib/bitvec.h"

using namespace P4;

/// Helper class to handle the @hash_mask() annotation.
class HashMaskAnnotations {
 public:
    HashMaskAnnotations(const IR::MAU::Table *tbl, const PhvInfo &phv) {
        key_hash_bits_masked = 0;
        for (auto &table_key : tbl->match_key) {
            for (auto &a : table_key->annotations->annotations) {
                if (a->name == "hash_mask") {
                    bitvec hash_mask = getBitvec(a);

                    le_bitrange bits = {0, 0};
                    const PHV::Field *field = phv.field(table_key->expr, &bits);
                    if (field) {
                        int masked = hash_mask.popcount();
                        if (masked > bits.size()) masked = bits.size();
                        key_hash_bits_masked += (bits.size() - masked);
                    }

                    const IR::Member *m = table_key->expr->to<IR::Member>();
                    if (m != nullptr) key_hash_masks[m->toString()] = hash_mask;
                    break;
                }
            }
        }
        if (LOGGING(5)) {
            LOG5("Hash mask annotations for table " << tbl->name << ":");
            for (auto &m : key_hash_masks) LOG5("  " << m.first << " : 0x" << std::hex << m.second);
        }
    }

    std::map<cstring, bitvec> hash_masks() { return key_hash_masks; }

    int hash_bits_masked() { return key_hash_bits_masked; }

 private:
    bitvec getBitvec(const IR::Annotation *annotation) {
        bitvec rv;
        if (annotation->expr.size() != 1) {
            error("%1% should contain a constant", annotation);
            return rv;
        }
        auto constant = annotation->expr[0]->to<IR::Constant>();
        if (constant == nullptr) {
            error("%1% should contain a constant", annotation);
            return rv;
        }
        int64_t c = constant->asUint64();
        rv.setraw(c);
        return rv;
    }

    // Vector of match keys annotated with @hash_mask()
    // along with their associated mask.
    std::map<cstring, bitvec> key_hash_masks;

    // Number of bits masked out with @hash_mask() throughout
    // all match keys in the table.
    int key_hash_bits_masked;
};

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_HASH_MASK_ANNOTATIONS_H_ */
