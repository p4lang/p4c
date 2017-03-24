/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef FRONTENDS_P4_14_HEADER_TYPE_H_
#define FRONTENDS_P4_14_HEADER_TYPE_H_

#include "ir/ir.h"

class CheckHeaderTypes : public Modifier {
    const IR::V1Program    *global;
 public:
    CheckHeaderTypes() { setName("CheckHeaderTypes"); }
    bool preorder(IR::V1Program *glob) override { global = glob; return true; }
    bool preorder(IR::Metadata *meta) override {
        if (auto type = global->get<IR::v1HeaderType>(meta->type_name))
            meta->type = type->as_metadata;
        else
            error("%s: No header type %s", meta->srcInfo, meta->type_name);
        return true; }
    bool preorder(IR::HeaderOrMetadata *hdr) override {
        if (auto type = global->get<IR::v1HeaderType>(hdr->type_name))
            hdr->type = type->as_header;
        else
            error("%s: No header type %s", hdr->srcInfo, hdr->type_name);
        return true; }
};

class HeaderTypeMaxLengthCalculator : public Modifier {
 public:
    bool preorder(IR::Type_StructLike *hdr_type) override;
};

#endif /* FRONTENDS_P4_14_HEADER_TYPE_H_ */
