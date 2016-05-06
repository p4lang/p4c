#include "header_type.h"

bool
HeaderTypeMaxLengthCalculator::preorder(IR::Type_StructLike *hdr_type) {
    IR::Vector<IR::Annotation> *annot = nullptr;
    auto *max_length = hdr_type->annotations->getSingle("max_length");
    if (!max_length) {
        unsigned len = 0;
        for (auto field : *hdr_type->fields)
            len += field->type->width_bits();
        max_length = new IR::Annotation("max_length", len);
        if (!annot) annot = hdr_type->annotations->annotations->clone();
        annot->push_back(max_length); }
    auto *length = hdr_type->annotations->getSingle("length");
    if (!length) {
        if (!annot) annot = hdr_type->annotations->annotations->clone();
        length = new IR::Annotation("length", max_length->expr);
        annot->push_back(length); }
    if (annot) hdr_type->annotations = new IR::Annotations(annot);
    return false;
}
