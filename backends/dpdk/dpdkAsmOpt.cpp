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

const IR::Expression* CopyPropagationAndElimination::getIrreplaceableExpr(cstring str,
                                                                        bool allowConst) {
    if (collectUseDef->dontEliminate.count(str))
        return nullptr;
    if (!str.startsWith("m."))
        return nullptr;
    auto expr = collectUseDef->replacementMap[str];
    const IR::Expression* prev = nullptr;
    cstring exprStr;
    if (expr) exprStr = expr->toString();
    while (expr != nullptr
        && (!allowConst ? expr->is<IR::Member>() : true)
        && str != exprStr
        && (!allowConst ? exprStr.startsWith("m.") : true)
        && collectUseDef->dontEliminate.count(exprStr) == 0
        && newUsesInfo[exprStr] == 0
        && (expr->is<IR::Member>() ? collectUseDef->haveSingleUseDef(exprStr) : true)) {
        prev = expr;
        expr = collectUseDef->replacementMap[exprStr];
        if (expr) exprStr = expr->toString();
    }
    if (expr != nullptr &&  allowConst)
        return expr;
    else if (expr != nullptr && expr->is<IR::Member>())
        return expr;
    else
        return prev;
}

const IR::Expression* CopyPropagationAndElimination::replaceIfCopy(const IR::Expression *expr,
                                                                    bool allowConst) {
    if (!expr)
        return expr;
    auto str = expr->toString();
    if (collectUseDef->haveSingleUseDef(str)) {
        if (auto rexpr = getIrreplaceableExpr(str, allowConst)) {
            return rexpr;
        }
    }
     if (!expr->is<IR::Constant>())
        newUsesInfo[expr->toString()]++;
    return expr;
}

const IR::DpdkAsmStatement* CopyPropagationAndElimination::elimCastOrMov(
    const IR::DpdkAsmStatement* stmt) {
    const IR::Expression* srcExpr = nullptr, *dstExpr = nullptr;
    if (stmt->is<IR::DpdkMovStatement>()) {
        auto mv = stmt->to<IR::DpdkMovStatement>();
        srcExpr = mv->src;
        dstExpr = mv->dst;
    } else {
        auto mv = stmt->to<IR::DpdkCastStatement>();
        srcExpr = mv->src;
        dstExpr = mv->dst;
    }
    auto src = srcExpr->toString();
    auto dst = dstExpr->toString();
    bool dropCopy = false;
    if (collectUseDef->dontEliminate.count(dst) == 0
            && dst.startsWith("m.")
            && newUsesInfo[dst] == 0
            && collectUseDef->haveSingleUseDef(dst)) {
        dropCopy = true;  // do nothing, drop the statement
    }
    if (!dropCopy) {
        auto r = replaceIfCopy(srcExpr);
        if (dst != r->toString() && dst.startsWith("m.")) {
            newUsesInfo[r->toString()]++;
            return new IR::DpdkMovStatement(dstExpr, r);
        } else {
            newUsesInfo[src]++;
            return stmt;
        }
    } else {
        return nullptr;
    }
}

IR::IndexedVector<IR::DpdkAsmStatement>
CopyPropagationAndElimination::copyPropAndDeadCodeElim(
    IR::IndexedVector<IR::DpdkAsmStatement> stmts) {
    IR::IndexedVector<IR::DpdkAsmStatement> instr;
    for (auto stmt1 = stmts.rbegin(); stmt1 != stmts.rend(); stmt1++) {
        auto stmt = *stmt1;
        if (stmt->is<IR::DpdkMovStatement>() || stmt->is<IR::DpdkCastStatement>()) {
            if (auto newMv = elimCastOrMov(stmt))
                instr.push_back(newMv);
        } else if (auto jc = stmt->to<IR::DpdkJmpLessStatement>()) {
            instr.push_back(new IR::DpdkJmpLessStatement(jc->label,
                        replaceIfCopy(jc->src1, false), replaceIfCopy(jc->src2)));
        } else if (auto jc = stmt->to<IR::DpdkJmpGreaterStatement>()) {
            instr.push_back(new IR::DpdkJmpGreaterStatement(jc->label,
                        replaceIfCopy(jc->src1, false), replaceIfCopy(jc->src2)));
        } else if (auto je = stmt->to<IR::DpdkJmpEqualStatement>()) {
            instr.push_back(new IR::DpdkJmpEqualStatement(je->label,
                        replaceIfCopy(je->src1, false), replaceIfCopy(je->src2)));
        } else if (auto jne = stmt->to<IR::DpdkJmpNotEqualStatement>()) {
            instr.push_back(new IR::DpdkJmpNotEqualStatement(jne->label,
                        replaceIfCopy(jne->src1, false), replaceIfCopy(jne->src2)));
       } else if (auto l = stmt->to<IR::DpdkLearnStatement>()) {
            instr.push_back(new IR::DpdkLearnStatement(l->action,
                        replaceIfCopy(l->timeout, false), replaceIfCopy(l->argument, false)));
       } else if (auto m = stmt->to<IR::DpdkMeterExecuteStatement>()) {
            instr.push_back(new IR::DpdkMeterExecuteStatement(m->meter,
                    replaceIfCopy(m->index), replaceIfCopy(m->length),
                    replaceIfCopy(m->color_in), replaceIfCopy(m->color_out)));
       } else {
            instr.push_back(stmt);
        }
    }

    IR::IndexedVector<IR::DpdkAsmStatement> instrr;
    for (auto stmt = instr.rbegin(); stmt != instr.rend(); stmt++) {
        instrr.push_back(*stmt);
    }
    return instrr;
}

size_t ShortenTokenLength::count = 0;
}  // namespace DPDK
