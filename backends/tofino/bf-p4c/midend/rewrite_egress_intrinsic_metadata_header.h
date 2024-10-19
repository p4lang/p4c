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

#ifndef BF_P4C_MIDEND_REWRITE_EGRESS_INTRINSIC_METADATA_HEADER_H_
#define BF_P4C_MIDEND_REWRITE_EGRESS_INTRINSIC_METADATA_HEADER_H_

#include "bf-p4c/device.h"

namespace P4 {
class TypeMap;
class ReferenceMap;
}  // namespace P4

namespace BFN {

/**
 * \ingroup midend
 * \brief Pass that updates egress intrinsic metadata.
 */
class RewriteEgressIntrinsicMetadataHeader : public PassManager {
    struct CollectUsedFields : public Inspector {
        std::set<cstring> used_fields;
        const IR::Type_Header *eg_intr_md = nullptr;

        bool preorder(const IR::Type_Header *type) override {
            if (type->name == "egress_intrinsic_metadata_t") {
                if (!eg_intr_md) eg_intr_md = type;
            }
            return false;
        }

        bool preorder(const IR::Member *member) override {
            if (auto hdr = member->expr->to<IR::PathExpression>()) {
                if (auto type = hdr->type->to<IR::Type_Header>()) {
                    if (type->name == "egress_intrinsic_metadata_t") {
                        used_fields.insert(member->member);
                    }
                }
            }

            return true;
        }
    };

    struct RewriteHeader : public Transform {
        const CollectUsedFields &used_fields;
        P4::TypeMap *typeMap;

        IR::IndexedVector<IR::StructField> new_fields;

        profile_t init_apply(const IR::Node *root) override {
            BUG_CHECK(used_fields.eg_intr_md, "egress_intrinsic_metadata_t not found?");

            const IR::StructField *prev = nullptr;
            const IR::Type *prev_type = nullptr;
            for (auto field : used_fields.eg_intr_md->fields) {
                if (used_fields.used_fields.count(field->name) || field->name == "egress_port") {
                    // "egress_port" is always there (not optional)
                    // we need to preserve each field's byte alignment as in original header
                    const IR::Type *canonicalType = nullptr;
                    if (field->type->is<IR::Type_Name>())
                        canonicalType = typeMap->getTypeType(field->type, true);
                    else
                        canonicalType = field->type;
                    if (canonicalType->width_bits() % 8) {
                        BUG_CHECK(
                            prev &&
                                (canonicalType->width_bits() + prev_type->width_bits()) % 8 == 0,
                            "%1% not padded to be byte-aligned in %2%", field->name,
                            "egress_intrinsic_metadata_t");
                        new_fields.push_back(prev);
                    }
                    new_fields.push_back(field);
                }
                prev = field;
                if (field->type->is<IR::Type_Name>())
                    prev_type = typeMap->getTypeType(field->type, true);
                else
                    prev_type = field->type;
            }

            unsigned total_size_in_bits = 0;
            for (auto field : new_fields) {
                const IR::Type *canonicalType;
                if (field->type->is<IR::Type_Name>())
                    canonicalType = typeMap->getTypeType(field->type, true);
                else
                    canonicalType = field->type;
                auto bits = canonicalType->to<IR::Type_Bits>();
                total_size_in_bits += bits->size;
            }

            BUG_CHECK(total_size_in_bits % 8 == 0,
                      "rewritten egress_intrinsic_metadata_t not byte-aligned?");

            // JBay requires the egress intrinsic metadata to be padded to 4-byte aligned
            // Minimum metadata lengh is 8B (JBay EPB uarch sec 6.1).
            if (Device::currentDevice() == Device::JBAY) {
                unsigned total_size_in_byte = total_size_in_bits / 8;
                unsigned tofino2_pad = ((total_size_in_byte + 3) / 4) * 4 - total_size_in_byte;
                if (total_size_in_byte + tofino2_pad < Device::egressIntrinsicMetadataMinLen())
                    tofino2_pad += Device::egressIntrinsicMetadataMinLen() -
                                   (total_size_in_byte + tofino2_pad);

                if (tofino2_pad) {
                    LOG4("tofino2 needs " << tofino2_pad << " bytes of padding");

                    auto *annots = new IR::Annotations({new IR::Annotation(IR::ID("padding"), {})});

                    const IR::StructField *tofino2_pad_field = new IR::StructField(
                        "__tofino2_pad_", annots, IR::Type::Bits::get(tofino2_pad * 8));

                    new_fields.push_back(tofino2_pad_field);
                }
            }

            return Transform::init_apply(root);
        }

        IR::Node *preorder(IR::Type_Header *type) override {
            if (type->name == "egress_intrinsic_metadata_t") {
                auto clone = type->clone();
                clone->fields = new_fields;
                LOG4("rewrite egress_intrinsic_metadata_t as:");
                LOG4(clone);
                return clone;
            }

            return type;
        }

        explicit RewriteHeader(const CollectUsedFields &used, P4::TypeMap *typeMap)
            : used_fields(used), typeMap(typeMap) {}
    };

 public:
    explicit RewriteEgressIntrinsicMetadataHeader(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        auto collectUsedFields = new CollectUsedFields;

        addPasses({
            collectUsedFields,
            new RewriteHeader(*collectUsedFields, typeMap),
            new P4::ClearTypeMap(typeMap),
            new BFN::TypeChecking(refMap, typeMap, true),
        });
    }
};

}  // namespace BFN

#endif /* BF_P4C_MIDEND_REWRITE_EGRESS_INTRINSIC_METADATA_HEADER_H_ */
