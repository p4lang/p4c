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

#include "bf-p4c/midend/check_header_alignment.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/device.h"
#include "frontends/p4/typeMap.h"
#include "lib/pad_alignment.h"
#include "lib/cstring.h"

#include "ir/ir.h"

namespace BFN {

bool CheckPadAssignment::preorder(const IR::AssignmentStatement* statement) {
    auto member = statement->left->to<IR::Member>();
    if (!member)
        return false;
    auto header_type = dynamic_cast<const IR::Type_StructLike *>(member->expr->type);
    if (!header_type)
        return false;
    auto field = header_type->getField(member->member.name);
    if (field && field->getAnnotation("padding"_cs)) {
        if (statement->srcInfo.isValid())
            warning(ErrorType::WARN_UNUSED, "%1%: Padding fields do not need to be explicitly "
                      "set. Also, setting these fields can introduce PHV constraints the compiler "
                      "may not be able to resolve.", statement);
    }

    return false;
}

/**
 * We do not need user to byte-align headers used in Mirror/Resubmit/Digest
 */
bool CheckHeaderAlignment::preorder(const IR::Type_Header* header) {
    auto canonicalType = typeMap->getTypeType(header, true);
    auto found = findFlexibleAnnotation(header);
    ERROR_CHECK(canonicalType->width_bits() % 8 == 0 || found,
                "%1% requires byte-aligned headers, but header %2% is not "
                "byte-aligned (has %3% bits)", Device::name(), header->name,
                canonicalType->width_bits());
    return false;
}

/**
 * Pad headers used in Mirror/Resubmit/Digest emit() method to byte boundaries.
 */
std::vector<cstring> FindPaddingCandidate::find_headers_to_pad(P4::MethodInstance* mi) {
    const auto all_hdrs = find_all_headers(mi);
    std::vector<cstring> retval;
    for (const auto & structlike : all_hdrs) {
        if (findFlexibleAnnotation(structlike)) retval.push_back(structlike->name);
    }
    return retval;
}

// This function find all headers in MethodInstance Parameters
std::vector<const IR::Type_StructLike*>
FindPaddingCandidate::find_all_headers(P4::MethodInstance* mi) {
    std::vector<const IR::Type_StructLike*> retval;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (p->direction != IR::Direction::In)
            continue;
        auto paramType = typeMap->getType(p, true);
        if (auto hdr = paramType->to<IR::Type_Header>()) {
            retval.push_back(hdr);
        } else if (auto st = paramType->to<IR::Type_Struct>()) {
            retval.push_back(st);
        }
    }
    return retval;
}

void FindPaddingCandidate::check_mirror(P4::MethodInstance* mi) {
    auto hdrs = find_headers_to_pad(mi);
    for (auto v : hdrs)
        headers_to_pad->insert(v);
}

void FindPaddingCandidate::check_resubmit(P4::MethodInstance* mi) {
    // This hdr means headers that have flexible fields, but it does not decide if it is resubmit
    const auto hdrs_to_pad = find_headers_to_pad(mi);
    for (const auto v : hdrs_to_pad)
        headers_to_pad->insert(v);
    // If it is in Resubmit()'s arguments, then it is a resubmit header regardless whether it has
    // flexible fields or not.
    const auto all_resubmit_hdrs = find_all_headers(mi);
    for (const auto v : all_resubmit_hdrs) {
        LOG5("add resubmit hdr: " << v->name);
        resubmit_headers->insert(v->name);
    }
}

void FindPaddingCandidate::check_digest(P4::MethodInstance* mi) {
    auto em = mi->to<P4::ExternMethod>();
    if (!em) return;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (auto hdr = p->type->to<IR::Type_Header>()) {
            auto decl = all_header_types->at(hdr->name);
            if (findFlexibleAnnotation(decl)) {
                LOG3("  found flexible annotation in " << decl->name <<
                        ", add as padding candidate");
                headers_to_pad->insert(decl->name); }
        }
    }
}

bool FindPaddingCandidate::preorder(const IR::MethodCallExpression* method) {
    auto mi = P4::MethodInstance::resolve(method, refMap, typeMap);
    if (auto *em = mi->to<P4::ExternMethod>()) {
        cstring externName = em->actualExternType->name;
        LOG5("externName " << externName);
        if (externName == "Mirror" || externName == "Pktgen") {
            check_mirror(mi);
        } else if (externName == "Resubmit") {
            check_resubmit(mi);
        } else if (externName == "Digest") {
            check_digest(mi);
        }
    }
    return false;
}

bool FindPaddingCandidate::preorder(const IR::Type_Header* type) {
    all_header_types->emplace(type->name, type);
    return false;
}

const IR::Node* AddPaddingFields::preorder(IR::Type_Header* header) {
    if (!headers_to_pad->count(header->name) &&
            !findFlexibleAnnotation(header))
        return header;
    IR::IndexedVector<IR::StructField> structFields;
    unsigned padFieldId = 0;
    int programmer_inserted_padding = 0;
    for (auto& field : header->fields) {
        const IR::Type* canonicalType;
        if (field->type->is<IR::Type_Name>())
            canonicalType = typeMap->getTypeType(field->type, true);
        else
            canonicalType = field->type;
        // If field has a @packed annotation, it is considered to be locked-in
        // to the position. this pass will not attempt to insert padding.  user
        // is responsible to ensure @packed fields does not incur phv
        // constraint.
        auto packed = field->getAnnotation("packed"_cs);
        if (packed != nullptr) {
            structFields.push_back(field);
            continue; }
        // If field has @padding annotation, it is treated as a padding field.
        // remove all other annotations on the field, especially the @flexible
        // annotation which could be introduced by header flattening.
        auto hidden = field->getAnnotation("padding"_cs);
        if (hidden != nullptr) {
            auto *fieldAnnotations = new IR::Annotations({
                new IR::Annotation(IR::ID("padding"), {})});
            structFields.push_back(new IR::StructField(field->name, fieldAnnotations, field->type));
            programmer_inserted_padding += canonicalType->width_bits();
            continue; }
        // Add padding field for every bridged metadata field to ensure that the resulting
        // header is byte aligned.
        auto total_bit_size = canonicalType->width_bits() + programmer_inserted_padding;
        const int alignment = getAlignment(total_bit_size);
        if (alignment != 0) {
            cstring padFieldName = "__pad_"_cs;
            padFieldName += cstring::to_cstring(padFieldId++);
            auto *fieldAnnotations = new IR::Annotations({
                    new IR::Annotation(IR::ID("padding"), {})});
            structFields.push_back(new IR::StructField(padFieldName, fieldAnnotations,
                                                       IR::Type::Bits::get(alignment)));
            programmer_inserted_padding = 0;
        }
        structFields.push_back(field);
    }

    auto retval = new IR::Type_Header(header->srcInfo, header->name,
            header->annotations, structFields);
    LOG6("rewrite flexible struct " << retval);
    return retval;
}

const IR::Node* AddPaddingFields::preorder(IR::StructExpression *st) {
    auto origType = typeMap->getType(st->structType)->to<IR::Type_Type>();
    BUG_CHECK(origType, "Expected %1% to be a type", st->structType);
    if (!origType->type->is<IR::Type_Header>())
        return st;

    auto type = st->type->to<IR::Type_Header>();
    BUG_CHECK(type, "Expected %1% to have a header type", st);

    auto type_name = type->name;
    if (!headers_to_pad->count(type_name))
        return st;

    LOG3(" start modifying ");
    IR::IndexedVector<IR::NamedExpression> components;
    // TODO: TypeChecking algorithm on IR::StructExpression
    // does not annotate the expression with the original IR::Type_Header.
    // Instead, it creates another IR::Type_Header instance with the same name
    // as the original header type, but without the annotation on the fields.
    // We had to maintain a map of all header types outselves and look up the
    // origin header type to access the annotations inside the header type.
    auto header = all_header_types->at(type_name);
    LOG3(" get header type " << header);
    int index = 0;
    int padFieldId = 0;
    int programmer_inserted_padding = 0;
    for (auto& field : header->fields) {
        const IR::Type* canonicalType;
        if (field->type->is<IR::Type_Name>())
            canonicalType = typeMap->getTypeType(field->type, true);
        else
            canonicalType = field->type;
        // Add padding field for every bridged metadata field to ensure that the resulting
        // header is byte aligned.
        LOG3(" canno type " << canonicalType);
        auto hidden = field->getAnnotation("padding"_cs);
        if (hidden != nullptr) {
            programmer_inserted_padding += canonicalType->width_bits();
        } else {
            int total_bit_size = canonicalType->width_bits() + programmer_inserted_padding;
            const int alignment = getAlignment(total_bit_size);
            LOG3("..." << total_bit_size);
            if (alignment != 0) {
                cstring padFieldName = "__pad_"_cs;
                padFieldName += cstring::to_cstring(padFieldId++);
                components.push_back(new IR::NamedExpression(padFieldName,
                            new IR::Constant(IR::Type::Bits::get(alignment), 0)));
                programmer_inserted_padding = 0;
            }
        }
        components.push_back(st->components.at(index));
        index++;
    }

    auto retval = new IR::StructExpression(st->srcInfo, st->structType, components);
    LOG6("rewrite field list to " << retval);
    return retval;
}

const IR::Node* TransformResubmitHeaders::preorder(IR::Type_Header* header) {
    /**
     * header used in resubmit is special, it must always be 64bit in size.
     * We use a special IR node so that flexible_packing knows how to deal with
     * the packing of resubmit header in the backend.
     */
    if (resubmit_headers->count(header->name)) {
        LOG3("rewrite resubmit header as fixed size");
        return new IR::BFN::Type_FixedSizeHeader(header->srcInfo, header->name,
            header->annotations, header->fields, Device::pardeSpec().bitResubmitSize()); }

    return header;
}

}  // namespace BFN
