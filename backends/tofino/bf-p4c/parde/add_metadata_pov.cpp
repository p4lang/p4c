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

#include "add_metadata_pov.h"

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

bool AddMetadataPOV::equiv(const IR::Expression *a, const IR::Expression *b) {
    if (auto field = phv.field(a)) return field == phv.field(b);
    return false;
}

IR::BFN::Pipe *AddMetadataPOV::preorder(IR::BFN::Pipe *pipe) {
    prune();
    for (auto &t : pipe->thread) {
        visit(t.deparser);
        dp = t.deparser->to<IR::BFN::Deparser>();
        parallel_visit(t.parsers);
        visit(t.mau);
    }
    return pipe;
}

IR::BFN::DeparserParameter *AddMetadataPOV::postorder(IR::BFN::DeparserParameter *param) {
    LOG5("Adding Metadata POV for deparser param: " << param);
    param->povBit = new IR::BFN::FieldLVal(new IR::TempVar(
        IR::Type::Bits::get(1), true, param->source->field->toString() + ".$valid"));
    return param;
}

IR::BFN::Digest *AddMetadataPOV::postorder(IR::BFN::Digest *digest) {
    if (!digest->selector) return digest;
    digest->povBit = new IR::BFN::FieldLVal(new IR::TempVar(
        IR::Type::Bits::get(1), true, digest->selector->field->toString() + ".$valid"));
    return digest;
}

IR::MAU::Primitive *AddMetadataPOV::create_pov_write(const IR::Expression *povBit, bool validate) {
    return new IR::MAU::Primitive("modify_field"_cs, povBit,
                                  new IR::Constant(IR::Type::Bits::get(1), (unsigned)validate));
}

IR::Node *AddMetadataPOV::insert_deparser_param_pov_write(const IR::MAU::Primitive *p,
                                                          bool validate) {
    Log::TempIndent indent;
    LOG5("Insert deparser param for pov write : " << p << ", validate: " << (validate ? "Y" : "N")
                                                  << indent);
    auto *dest = p->operands.at(0);
    for (auto *param : dp->params) {
        if (equiv(dest, param->source->field)) {
            auto pov_write = create_pov_write(param->povBit->field, validate);
            LOG5("Inserting param for pov write: " << pov_write);
            if (validate)
                return new IR::Vector<IR::MAU::Primitive>({p, pov_write});
            else
                return pov_write;
        }
    }
    LOG5("No parameter inserted");
    return nullptr;
}

IR::Node *AddMetadataPOV::insert_deparser_digest_pov_write(const IR::MAU::Primitive *p,
                                                           bool validate) {
    auto *dest = p->operands.at(0);
    for (auto &item : dp->digests) {
        auto *digest = item.second;
        if (equiv(dest, digest->selector->field)) {
            auto pov_write = create_pov_write(digest->povBit->field, validate);
            if (validate)
                return new IR::Vector<IR::MAU::Primitive>({p, pov_write});
            else
                return pov_write;
        }
    }
    return nullptr;
}

/**
 * @brief Convert the 'is_validated' extern to $valid bit read.
 *
 * @param p represents the method call to is_validated()
 * @return reference to <field>.$valid
 */
IR::Node *AddMetadataPOV::insert_field_pov_read(const IR::MAU::Primitive *p) {
    if (p->is<IR::MAU::TypedPrimitive>()) {
        return new IR::TempVar(IR::Type::Bits::get(1), true,
                               p->operands.at(0)->toString() + ".$valid");
    }
    return nullptr;
}

IR::Node *AddMetadataPOV::postorder(IR::MAU::Primitive *p) {
    if (p->name == "modify_field") {
        if (auto rv = insert_deparser_param_pov_write(p, true)) return rv;
        if (auto rv = insert_deparser_digest_pov_write(p, true)) return rv;
    } else if (p->name == "invalidate") {
        if (auto rv = insert_deparser_param_pov_write(p, false)) return rv;
        if (auto rv = insert_deparser_digest_pov_write(p, false)) return rv;
    } else if (p->name == "is_validated") {
        if (auto rv = insert_field_pov_read(p)) return rv;
    }

    return p;
}

IR::Node *AddMetadataPOV::postorder(IR::BFN::Extract *e) {
    for (auto *param : dp->params) {
        if (param->povBit) {
            if (auto lval = e->dest->to<IR::BFN::FieldLVal>()) {
                if (equiv(lval->field, param->source->field))
                    return new IR::Vector<IR::BFN::ParserPrimitive>(
                        {e, new IR::BFN::Extract(param->povBit, new IR::BFN::ConstantRVal(1))});
            }
        }
    }
    return e;
}
