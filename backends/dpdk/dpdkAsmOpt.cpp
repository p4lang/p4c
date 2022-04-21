/*
Copyright 2020 Intel Corp.

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

#include "dpdkAsmOpt.h"
#include "dpdkUtils.h"

namespace DPDK {
// The assumption is compiler can only produce forward jumps.
const IR::IndexedVector<IR::DpdkAsmStatement> *RemoveRedundantLabel::removeRedundantLabel(
                                       const IR::IndexedVector<IR::DpdkAsmStatement> &s) {
    IR::IndexedVector<IR::DpdkAsmStatement> used_labels;
    for (auto stmt : s) {
        if (auto jmp = stmt->to<IR::DpdkJmpStatement>()) {
            bool found = false;
            for (auto label : used_labels) {
                if (label->to<IR::DpdkJmpStatement>()->label ==
                    jmp->label) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                used_labels.push_back(stmt);
            }
        }
    }
    auto new_l = new IR::IndexedVector<IR::DpdkAsmStatement>;
    for (auto stmt : s) {
        if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
            bool found = false;
            for (auto jmp_label : used_labels) {
                if (jmp_label->to<IR::DpdkJmpStatement>()->label ==
                    label->label) {
                    found = true;
                    break;
                }
            }
            if (found) {
                new_l->push_back(stmt);
            }
        } else {
            new_l->push_back(stmt);
        }
    }
    return new_l;
}

const IR::IndexedVector<IR::DpdkAsmStatement> *RemoveConsecutiveJmpAndLabel::removeJmpAndLabel(
                                            const IR::IndexedVector<IR::DpdkAsmStatement> &s) {
    const IR::DpdkJmpStatement *cache = nullptr;
    auto new_l = new IR::IndexedVector<IR::DpdkAsmStatement>;
    for (auto stmt : s) {
        if (auto jmp = stmt->to<IR::DpdkJmpStatement>()) {
            if (cache)
                new_l->push_back(cache);
            cache = jmp;
        } else if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
            if (!cache) {
                new_l->push_back(stmt);
            } else if (cache->label != label->label) {
                new_l->push_back(cache);
                cache = nullptr;
                new_l->push_back(stmt);
            } else {
                new_l->push_back(stmt);
                cache = nullptr;
            }
        } else {
            if (cache) {
                new_l->push_back(cache);
                cache = nullptr;
            }
            new_l->push_back(stmt);
        }
    }
    // Do not remove jump to LABEL_DROP as LABEL_DROP is not part of statement list and
    // should not be optimized here.
    if (cache && cache->label == "LABEL_DROP")
        new_l->push_back(cache);
    return  new_l;
}

const IR::IndexedVector<IR::DpdkAsmStatement> *ThreadJumps::threadJumps(
                     const IR::IndexedVector<IR::DpdkAsmStatement> &s) {
    std::map<const cstring, cstring> label_map;
    const IR::DpdkLabelStatement *cache = nullptr;
    for (auto stmt : s) {
        if (!cache) {
            if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
                LOG1("label " << label);
                cache = label;
            }
        } else {
            if (auto jmp = stmt->to<IR::DpdkJmpLabelStatement>()) {
                LOG1("emplace " << cache->label << " " << jmp->label);
                label_map.emplace(cache->label, jmp->label);
            } else {
                cache = nullptr;
            }
        }
    }
    auto new_l = new IR::IndexedVector<IR::DpdkAsmStatement>;
    for (auto stmt : s) {
        if (auto jmp = stmt->to<IR::DpdkJmpStatement>()) {
            auto res = label_map.find(jmp->label);
            if (res != label_map.end()) {
                ((IR::DpdkJmpStatement *)stmt)->label = res->second;
                new_l->push_back(stmt);
            } else {
                new_l->push_back(stmt);
            }
        } else {
            new_l->push_back(stmt);
        }
    }
    return new_l;
}


const IR::IndexedVector<IR::DpdkAsmStatement> *RemoveLabelAfterLabel::removeLabelAfterLabel(
                                         const IR::IndexedVector<IR::DpdkAsmStatement> &s) {
    std::map<const cstring, cstring> label_map;
    const IR::DpdkLabelStatement *cache = nullptr;
    for (auto stmt : s) {
        if (!cache) {
            if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
                cache = label;
            }
        } else {
            if (auto label = stmt->to<IR::DpdkLabelStatement>()) {
                label_map.emplace(cache->label, label->label);
            } else {
                cache = nullptr;
            }
        }
    }

    auto new_l = new IR::IndexedVector<IR::DpdkAsmStatement>;
    for (auto stmt : s) {
        if (auto jmp = stmt->to<IR::DpdkJmpStatement>()) {
            auto res = label_map.find(jmp->label);
            if (res != label_map.end()) {
                ((IR::DpdkJmpStatement *)stmt)->label = res->second;
                new_l->push_back(stmt);
            } else {
                new_l->push_back(stmt);
            }
        } else {
            new_l->push_back(stmt);
        }
    }
    return new_l;
}

bool RemoveUnusedMetadataFields::isByteSizeField(const IR::Type *field_type) {
    // DPDK implements bool and error types as bit<8>
    if (field_type->is<IR::Type_Boolean>() || field_type->is<IR::Type_Error>())
        return true;

    if (auto t = field_type->to<IR::Type_Name>()) {
        if (t->path->name != "error")
            return true;
    }
    return false;
}

const IR::Node* RemoveUnusedMetadataFields::preorder(IR::DpdkAsmProgram *p) {
    IR::IndexedVector<IR::DpdkStructType> usedStruct;
    for (auto st : p->structType) {
        if (isMetadataStruct(st)) {
                    IR::IndexedVector<IR::StructField> usedMetadataFields;
                    for (auto field : st->fields) {
                        if (used_fields.count(field->name.name)) {
                            if (isByteSizeField(field->type)) {
                                usedMetadataFields.push_back(new IR::StructField(
                                                      IR::ID(field->name), IR::Type_Bits::get(8)));
                            } else {
                                usedMetadataFields.push_back(field);
                            }
                        }
                    }
                    auto newSt = new IR::DpdkStructType(st->srcInfo, st->name,
                                                   st->annotations, usedMetadataFields);
                    usedStruct.push_back(newSt);
        } else {
            usedStruct.push_back(st);
        }
    }
    p->structType = usedStruct;
    return p;
}

int ValidateTableKeys::getFieldSizeBits(const IR::Type *field_type) {
    if (auto t = field_type->to<IR::Type_Bits>()) {
        return t->width_bits();
    } else if (field_type->is<IR::Type_Boolean>() ||
        field_type->is<IR::Type_Error>()) {
        return 8;
    } else if (auto t = field_type->to<IR::Type_Name>()) {
        if (t->path->name == "error") {
            return 8;
        } else {
            return -1;
        }
    }
    return -1;
}

bool ValidateTableKeys::preorder(const IR::DpdkAsmProgram *p) {
    const IR::DpdkStructType *metaStruct = nullptr;
    for (auto st : p->structType) {
        if (isMetadataStruct(st)) {
            metaStruct = st;
            break;
        }
    }
    for (auto tbl : p->tables) {
        int min, max, size_max_field = 0;
        auto keys = tbl->match_keys;
        if (!keys || keys->keyElements.size() == 0) {
            return false;
        }
        min = max = -1;
        for (auto key : keys->keyElements) {
            BUG_CHECK(key->expression->is<IR::Member>(), "Table keys must be a structure field. "
                                                          "%1% is not a structure field", key);
            auto keyMem = key->expression->to<IR::Member>();
            auto type = keyMem->expr->type;
            if (type->is<IR::Type_Struct>()
                && isMetadataStruct(type->to<IR::Type_Struct>())) {
                auto offset = metaStruct->getFieldBitOffset(keyMem->member.name);
                if (min == -1 || min > offset)
                    min = offset;
                if (max == -1 || max < offset) {
                    max = offset;
                    auto field_type = key->expression->type;
                    size_max_field = getFieldSizeBits(field_type);
                    if (size_max_field == -1) {
                        BUG("Unexpected type %1%", field_type->node_type_name());
                        return false;
                    }
                 }
             }
            if ((max + size_max_field - min) > DPDK_TABLE_MAX_KEY_SIZE) {
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%: All table keys together with"
                        " holes in the underlying structure should fit in 64 bytes", tbl->name);
                return false;
            }
        }
    }
    return false;
}

size_t ShortenTokenLength::count = 0;
}  // namespace DPDK
