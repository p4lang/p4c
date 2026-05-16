/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_14_HEADER_TYPE_H_
#define FRONTENDS_P4_14_HEADER_TYPE_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

class CheckHeaderTypes : public Modifier {
    const IR::V1Program *global = nullptr;

 public:
    CheckHeaderTypes() { setName("CheckHeaderTypes"); }
    bool preorder(IR::V1Program *glob) override {
        global = glob;
        return true;
    }
    bool preorder(IR::Metadata *meta) override {
        if (auto type = global->get<IR::v1HeaderType>(meta->type_name))
            meta->type = type->as_metadata;
        else
            error(ErrorType::ERR_TYPE_ERROR, "%s: No header type %s", meta->srcInfo,
                  meta->type_name);
        return true;
    }
    bool preorder(IR::HeaderOrMetadata *hdr) override {
        if (auto type = global->get<IR::v1HeaderType>(hdr->type_name))
            hdr->type = type->as_header;
        else
            error(ErrorType::ERR_TYPE_ERROR, "%s: No header type %s", hdr->srcInfo, hdr->type_name);
        return true;
    }
};

class HeaderTypeMaxLengthCalculator : public Modifier {
 public:
    bool preorder(IR::Type_StructLike *hdr_type) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_14_HEADER_TYPE_H_ */
