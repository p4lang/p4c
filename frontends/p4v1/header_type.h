#ifndef FRONTENDS_P4V1_HEADER_TYPE_H_
#define FRONTENDS_P4V1_HEADER_TYPE_H_

#include "ir/ir.h"

class CheckHeaderTypes : public Modifier {
    const IR::Global    *global;
    bool preorder(IR::Global *glob) override { global = glob; return true; }
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

#endif /* FRONTENDS_P4V1_HEADER_TYPE_H_ */
