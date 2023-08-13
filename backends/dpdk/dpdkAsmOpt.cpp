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

#include <map>
#include <stdexcept>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "dpdkUtils.h"
#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "ir/declaration.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace DPDK {
// The assumption is compiler can only produce forward jumps.
const IR::IndexedVector<IR::DpdkAsmStatement> *RemoveRedundantLabel::removeRedundantLabel(
    const IR::IndexedVector<IR::DpdkAsmStatement> &s) {
    IR::IndexedVector<IR::DpdkAsmStatement> used_labels;
    for (auto stmt : s) {
        if (auto jmp = stmt->to<IR::DpdkJmpStatement>()) {
            bool found = false;
            for (auto label : used_labels) {
                if (label->to<IR::DpdkJmpStatement>()->label == jmp->label) {
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
                if (jmp_label->to<IR::DpdkJmpStatement>()->label == label->label) {
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
            if (cache) new_l->push_back(cache);
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
    if (cache && cache->label == "LABEL_DROP") new_l->push_back(cache);
    return new_l;
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
    if (field_type->is<IR::Type_Boolean>() || field_type->is<IR::Type_Error>()) return true;

    if (auto t = field_type->to<IR::Type_Name>()) {
        if (t->path->name != "error") return true;
    }
    return false;
}

const IR::Node *RemoveUnusedMetadataFields::preorder(IR::DpdkAsmProgram *p) {
    IR::IndexedVector<IR::DpdkStructType> usedStruct;
    for (auto st : p->structType) {
        if (isMetadataStruct(st)) {
            IR::IndexedVector<IR::StructField> usedMetadataFields;
            for (auto field : st->fields) {
                if (used_fields.count(field->name.name)) {
                    if (isByteSizeField(field->type)) {
                        usedMetadataFields.push_back(
                            new IR::StructField(IR::ID(field->name), IR::Type_Bits::get(8)));
                    } else {
                        usedMetadataFields.push_back(field);
                    }
                }
            }
            auto newSt =
                new IR::DpdkStructType(st->srcInfo, st->name, st->annotations, usedMetadataFields);
            usedStruct.push_back(newSt);
        } else {
            usedStruct.push_back(st);
        }
    }
    p->structType = usedStruct;
    return p;
}

const IR::Expression *CopyPropagationAndElimination::getIrreplaceableExpr(cstring str,
                                                                          bool allowConst) {
    if (collectUseDef->dontEliminate.count(str) != 0) return nullptr;
    if (!str.startsWith("m.")) return nullptr;
    auto expr = collectUseDef->replacementMap[str];
    const IR::Expression *prev = nullptr;
    cstring exprStr;
    if (expr) exprStr = expr->toString();
    while (expr != nullptr && (!allowConst ? expr->is<IR::Member>() : true) && str != exprStr &&
           (!allowConst ? exprStr.startsWith("m.") : true) &&
           collectUseDef->dontEliminate.count(exprStr) == 0 && newUsesInfo[exprStr] == 0 &&
           (expr->is<IR::Member>() ? collectUseDef->haveSingleUseDef(exprStr) : true)) {
        prev = expr;
        expr = collectUseDef->replacementMap[exprStr];
        if (expr) exprStr = expr->toString();
    }
    if (expr != nullptr && allowConst)
        return expr;
    else if (expr != nullptr && expr->is<IR::Member>())
        return expr;
    else
        return prev;
}

const IR::Expression *CopyPropagationAndElimination::replaceIfCopy(const IR::Expression *expr,
                                                                   bool allowConst) {
    if (!expr) return expr;
    auto str = expr->toString();
    if (collectUseDef->haveSingleUseDef(str)) {
        if (auto rexpr = getIrreplaceableExpr(str, allowConst)) {
            return rexpr;
        }
    }
    if (!expr->is<IR::Constant>()) newUsesInfo[expr->toString()]++;
    return expr;
}

const IR::DpdkAsmStatement *CopyPropagationAndElimination::elimCastOrMov(
    const IR::DpdkAsmStatement *stmt) {
    const IR::Expression *srcExpr = nullptr, *dstExpr = nullptr;
    if (auto mv = stmt->to<IR::DpdkMovStatement>()) {
        srcExpr = mv->src;
        dstExpr = mv->dst;
    } else if (auto mv = stmt->to<IR::DpdkCastStatement>()) {
        srcExpr = mv->src;
        dstExpr = mv->dst;
    } else {
        return stmt;
    }
    auto src = srcExpr->toString();
    auto dst = dstExpr->toString();
    bool dropCopy = false;
    if (collectUseDef->dontEliminate.count(dst) == 0 && dst.startsWith("m.") &&
        newUsesInfo[dst] == 0 && collectUseDef->haveSingleUseDef(dst)) {
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

IR::IndexedVector<IR::DpdkAsmStatement> CopyPropagationAndElimination::copyPropAndDeadCodeElim(
    IR::IndexedVector<IR::DpdkAsmStatement> stmts) {
    IR::IndexedVector<IR::DpdkAsmStatement> instr;
    for (auto stmt1 = stmts.rbegin(); stmt1 != stmts.rend(); stmt1++) {
        auto stmt = *stmt1;
        if (stmt->is<IR::DpdkMovStatement>() || stmt->is<IR::DpdkCastStatement>()) {
            if (auto newMv = elimCastOrMov(stmt)) instr.push_back(newMv);
        } else if (auto jc = stmt->to<IR::DpdkJmpLessStatement>()) {
            instr.push_back(new IR::DpdkJmpLessStatement(jc->label, replaceIfCopy(jc->src1, false),
                                                         replaceIfCopy(jc->src2)));
        } else if (auto jc = stmt->to<IR::DpdkJmpLessOrEqualStatement>()) {
            instr.push_back(new IR::DpdkJmpLessOrEqualStatement(
                jc->label, replaceIfCopy(jc->src1, false), replaceIfCopy(jc->src2)));
        } else if (auto jc = stmt->to<IR::DpdkJmpGreaterStatement>()) {
            instr.push_back(new IR::DpdkJmpGreaterStatement(
                jc->label, replaceIfCopy(jc->src1, false), replaceIfCopy(jc->src2)));
        } else if (auto jc = stmt->to<IR::DpdkJmpGreaterEqualStatement>()) {
            instr.push_back(new IR::DpdkJmpGreaterEqualStatement(
                jc->label, replaceIfCopy(jc->src1, false), replaceIfCopy(jc->src2)));
        } else if (auto je = stmt->to<IR::DpdkJmpEqualStatement>()) {
            instr.push_back(new IR::DpdkJmpEqualStatement(je->label, replaceIfCopy(je->src1, false),
                                                          replaceIfCopy(je->src2)));
        } else if (auto jne = stmt->to<IR::DpdkJmpNotEqualStatement>()) {
            instr.push_back(new IR::DpdkJmpNotEqualStatement(
                jne->label, replaceIfCopy(jne->src1, false), replaceIfCopy(jne->src2)));
        } else if (auto m = stmt->to<IR::DpdkMeterExecuteStatement>()) {
            instr.push_back(new IR::DpdkMeterExecuteStatement(
                m->meter, replaceIfCopy(m->index), replaceIfCopy(m->length),
                replaceIfCopy(m->color_in), replaceIfCopy(m->color_out)));
        } else if (auto c = stmt->to<IR::DpdkCounterCountStatement>()) {
            instr.push_back(new IR::DpdkCounterCountStatement(c->counter, replaceIfCopy(c->index),
                                                              replaceIfCopy(c->incr)));
        } else if (auto neg = stmt->to<IR::DpdkNegStatement>()) {
            instr.push_back(
                new IR::DpdkNegStatement(replaceIfCopy(neg->dst, false), replaceIfCopy(neg->src)));
        } else if (auto cmpl = stmt->to<IR::DpdkCmplStatement>()) {
            instr.push_back(new IR::DpdkCmplStatement(replaceIfCopy(cmpl->dst, false),
                                                      replaceIfCopy(cmpl->src)));
        } else if (auto lnot = stmt->to<IR::DpdkLNotStatement>()) {
            instr.push_back(new IR::DpdkLNotStatement(replaceIfCopy(lnot->dst, false),
                                                      replaceIfCopy(lnot->src)));
        } else if (auto add = stmt->to<IR::DpdkAddStatement>()) {
            instr.push_back(new IR::DpdkAddStatement(replaceIfCopy(add->dst, false),
                                                     replaceIfCopy(add->src1, false),
                                                     replaceIfCopy(add->src2)));
        } else if (auto shl = stmt->to<IR::DpdkShlStatement>()) {
            instr.push_back(new IR::DpdkShlStatement(replaceIfCopy(shl->dst, false),
                                                     replaceIfCopy(shl->src1, false),
                                                     replaceIfCopy(shl->src2)));
        } else if (auto an = stmt->to<IR::DpdkAndStatement>()) {
            instr.push_back(new IR::DpdkAndStatement(replaceIfCopy(an->dst, false),
                                                     replaceIfCopy(an->src1, false),
                                                     replaceIfCopy(an->src2)));
        } else if (auto shr = stmt->to<IR::DpdkShrStatement>()) {
            instr.push_back(new IR::DpdkShrStatement(replaceIfCopy(shr->dst, false),
                                                     replaceIfCopy(shr->src1, false),
                                                     replaceIfCopy(shr->src2)));
        } else if (auto sub = stmt->to<IR::DpdkSubStatement>()) {
            instr.push_back(new IR::DpdkSubStatement(replaceIfCopy(sub->dst, false),
                                                     replaceIfCopy(sub->src1, false),
                                                     replaceIfCopy(sub->src2)));
        } else if (auto or1 = stmt->to<IR::DpdkOrStatement>()) {
            instr.push_back(new IR::DpdkOrStatement(replaceIfCopy(or1->dst, false),
                                                    replaceIfCopy(or1->src1, false),
                                                    replaceIfCopy(or1->src2)));
        } else if (auto eq = stmt->to<IR::DpdkEquStatement>()) {
            instr.push_back(new IR::DpdkEquStatement(replaceIfCopy(eq->dst, false),
                                                     replaceIfCopy(eq->src1, false),
                                                     replaceIfCopy(eq->src2)));
        } else if (auto xor1 = stmt->to<IR::DpdkXorStatement>()) {
            instr.push_back(new IR::DpdkXorStatement(replaceIfCopy(xor1->dst, false),
                                                     replaceIfCopy(xor1->src1, false),
                                                     replaceIfCopy(xor1->src2)));
        } else if (auto cmp = stmt->to<IR::DpdkCmpStatement>()) {
            instr.push_back(new IR::DpdkCmpStatement(replaceIfCopy(cmp->dst, false),
                                                     replaceIfCopy(cmp->src1, false),
                                                     replaceIfCopy(cmp->src2)));
        } else if (auto and1 = stmt->to<IR::DpdkLAndStatement>()) {
            instr.push_back(new IR::DpdkLAndStatement(replaceIfCopy(and1->dst, false),
                                                      replaceIfCopy(and1->src1, false),
                                                      replaceIfCopy(and1->src2)));
        } else if (auto lor = stmt->to<IR::DpdkLOrStatement>()) {
            instr.push_back(new IR::DpdkLOrStatement(replaceIfCopy(lor->dst, false),
                                                     replaceIfCopy(lor->src1, false),
                                                     replaceIfCopy(lor->src2)));
        } else if (auto leq = stmt->to<IR::DpdkLeqStatement>()) {
            instr.push_back(new IR::DpdkLeqStatement(replaceIfCopy(leq->dst, false),
                                                     replaceIfCopy(leq->src1, false),
                                                     replaceIfCopy(leq->src2)));
        } else if (auto lss = stmt->to<IR::DpdkLssStatement>()) {
            instr.push_back(new IR::DpdkLssStatement(replaceIfCopy(lss->dst, false),
                                                     replaceIfCopy(lss->src1, false),
                                                     replaceIfCopy(lss->src2)));
        } else if (auto grt = stmt->to<IR::DpdkGrtStatement>()) {
            instr.push_back(new IR::DpdkGrtStatement(replaceIfCopy(grt->dst, false),
                                                     replaceIfCopy(grt->src1, false),
                                                     replaceIfCopy(grt->src2)));
        } else if (auto geq = stmt->to<IR::DpdkGeqStatement>()) {
            instr.push_back(new IR::DpdkGeqStatement(replaceIfCopy(geq->dst, false),
                                                     replaceIfCopy(geq->src1, false),
                                                     replaceIfCopy(geq->src2)));
        } else if (auto neq = stmt->to<IR::DpdkNeqStatement>()) {
            instr.push_back(new IR::DpdkNeqStatement(replaceIfCopy(neq->dst, false),
                                                     replaceIfCopy(neq->src1, false),
                                                     replaceIfCopy(neq->src2)));
        } else if (auto recd = stmt->to<IR::DpdkRecircidStatement>()) {
            instr.push_back(new IR::DpdkRecircidStatement(replaceIfCopy(recd->pass, false)));
        } else if (auto mdecl = stmt->to<IR::DpdkMeterDeclStatement>()) {
            instr.push_back(
                new IR::DpdkMeterDeclStatement(mdecl->meter, replaceIfCopy(mdecl->size)));
        } else if (auto rdecl = stmt->to<IR::DpdkRegisterDeclStatement>()) {
            instr.push_back(new IR::DpdkRegisterDeclStatement(
                rdecl->reg, replaceIfCopy(rdecl->size), replaceIfCopy(rdecl->init_val)));
        } else if (auto rrd = stmt->to<IR::DpdkRegisterReadStatement>()) {
            instr.push_back(new IR::DpdkRegisterReadStatement(replaceIfCopy(rrd->dst, false),
                                                              rrd->reg, replaceIfCopy(rrd->index)));
        } else if (auto rrw = stmt->to<IR::DpdkRegisterWriteStatement>()) {
            instr.push_back(new IR::DpdkRegisterWriteStatement(rrw->reg, replaceIfCopy(rrw->index),
                                                               replaceIfCopy(rrw->src)));
        } else if (auto ca = stmt->to<IR::DpdkChecksumAddStatement>()) {
            instr.push_back(new IR::DpdkChecksumAddStatement(ca->csum, ca->intermediate_value,
                                                             replaceIfCopy(ca->field, false)));
        } else if (auto ca = stmt->to<IR::DpdkChecksumSubStatement>()) {
            instr.push_back(new IR::DpdkChecksumSubStatement(ca->csum, ca->intermediate_value,
                                                             replaceIfCopy(ca->field, false)));
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

cstring EmitDpdkTableConfig::getKeyMatchType(const IR::KeyElement *ke, P4::ReferenceMap *refMap) {
    auto path = ke->matchType->path;
    auto mt = refMap->getDeclaration(path, true)->to<IR::Declaration_ID>();
    BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
    return mt->name.name;
}

int EmitDpdkTableConfig::getTypeWidth(const IR::Type *type, P4::TypeMap *typeMap) {
    return typeMap->widthBits(type, type->getNode(), false);
}

void EmitDpdkTableConfig::print(cstring str, cstring sep) { dpdkTableConfigFile << str << sep; }

void EmitDpdkTableConfig::print(big_int str, cstring sep) {
    try {
        dpdkTableConfigFile << "0x" << std::hex << str << sep;
    } catch (const std::runtime_error &re) {
        dpdkTableConfigFile << std::dec << str << sep;
    }
}

big_int EmitDpdkTableConfig::convertSimpleKeyExpressionToBigInt(const IR::Expression *k,
                                                                int keyWidth,
                                                                P4::TypeMap *typeMap) {
    if (k->is<IR::Constant>()) {
        return k->to<IR::Constant>()->value;
    } else if (k->is<IR::BoolLiteral>()) {
        return static_cast<big_int>(k->to<IR::BoolLiteral>()->value ? 1 : 0);
    } else if (k->is<IR::Member>()) {  // handle SerEnum members
        auto mem = k->to<IR::Member>();
        auto se = mem->type->to<IR::Type_SerEnum>();
        auto ei = P4::EnumInstance::resolve(mem, typeMap);
        if (!ei) return -1;
        if (auto sei = ei->to<P4::SerEnumInstance>()) {
            auto type = sei->value->to<IR::Constant>();
            auto w = se->type->width_bits();
            BUG_CHECK(w == keyWidth, "SerEnum bitwidth mismatch");
            return type->value;
        }
        ::error(ErrorType::ERR_INVALID, "%1% invalid Member key expression", k);
        return -1;
    } else {
        ::error(ErrorType::ERR_INVALID, "%1% invalid key expression", k);
        return -1;
    }
}

void EmitDpdkTableConfig::addAction(const IR::Expression *actionRef, P4::ReferenceMap *refMap,
                                    P4::TypeMap *typeMap) {
    auto actionCall = actionRef->to<IR::MethodCallExpression>();
    auto method = actionCall->method->to<IR::PathExpression>()->path;
    const IR::Path *origMethod = nullptr;
    if (ShortenTokenLength::origNameMap.count(method->name) > 0) {
        origMethod = new IR::Path(ShortenTokenLength::origNameMap[method->name]);
    } else {
        origMethod = method;
    }
    auto decl = refMap->getDeclaration(origMethod, true);
    auto actionDecl = decl->to<IR::P4Action>();
    cstring actionName;
    if (newNameMap.count(actionDecl->name.name) > 0)
        actionName = newNameMap[actionDecl->name.name];
    else
        actionName = actionDecl->name.name;
    print(actionName, " ");
    if (actionDecl->parameters->parameters.size() == 1) {
        std::vector<cstring> paramNames;
        std::vector<big_int> argVals;
        auto parameter = actionDecl->parameters->parameters.at(0);
        auto type = typeMap->getType(parameter);
        if (auto actArg = type->to<IR::Type_Struct>()) {
            for (auto f : actArg->fields) paramNames.push_back(f->name);
        }
        for (auto args : *actionCall->arguments) {
            for (auto arg : args->expression->to<IR::ListExpression>()->components) {
                auto ei = P4::EnumInstance::resolve(arg, typeMap);
                if (arg->is<IR::Constant>()) {
                    auto constant = arg->to<IR::Constant>();
                    auto argValue = constant->value;
                    argVals.push_back(argValue);
                } else if (arg->is<IR::BoolLiteral>()) {
                    auto constant = arg->to<IR::BoolLiteral>();
                    auto argValue = static_cast<big_int>(constant->value ? 1 : 0);
                    argVals.push_back(argValue);
                } else if (ei != nullptr && ei->is<P4::SerEnumInstance>()) {
                    auto sei = ei->to<P4::SerEnumInstance>();
                    auto argValue = sei->value->to<IR::Constant>();
                    argVals.push_back(argValue->value);
                } else {
                    ::error(ErrorType::ERR_UNSUPPORTED, "%1% unsupported argument expression", arg);
                    continue;
                }
            }
        }

        for (size_t i = 0; i < argVals.size(); i++) {
            print(paramNames[i], " ");
            print(argVals[i], " ");
        }
    }
}

void EmitDpdkTableConfig::addExact(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap) {
    if (k->is<IR::DefaultExpression>()) {
        print("0x0/0x0", " ");
    } else {
        auto value = convertSimpleKeyExpressionToBigInt(k, keyWidth, typeMap);
        print(value, " ");
    }
}

void EmitDpdkTableConfig::addLpm(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap) {
    big_int valueStr;
    big_int mask;
    if (k->is<IR::DefaultExpression>()) {
        valueStr = 0x0;
        mask = 0x0;
    } else if (k->is<IR::Mask>()) {
        auto km = k->to<IR::Mask>();
        auto value = convertSimpleKeyExpressionToBigInt(km->left, keyWidth, typeMap);
        auto trailing_zeros = [keyWidth](const big_int &n) -> int {
            return (n == 0) ? keyWidth : boost::multiprecision::lsb(n);
        };
        auto count_ones = [](const big_int &n) -> int { return bitcount(n); };
        mask = km->right->to<IR::Constant>()->value;
        auto len = trailing_zeros(mask);
        if (len + count_ones(mask) != keyWidth) {  // any remaining 0s in the prefix?
            ::error(ErrorType::ERR_INVALID, "%1% invalid mask for LPM key", k);
            return;
        }
        if ((value & mask) != value) {
            ::warning(ErrorType::WARN_MISMATCH,
                      "P4Runtime requires that LPM matches have masked-off bits set to 0, "
                      "updating value %1% to conform to the P4Runtime specification",
                      km->left);
            value &= mask;
        }
        valueStr = value;
    } else {
        valueStr = convertSimpleKeyExpressionToBigInt(k, keyWidth, typeMap);
    }
    print(valueStr, "/");
    print(mask, " ");
}

void EmitDpdkTableConfig::addTernary(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap) {
    big_int valueStr;
    big_int maskStr;
    if (k->is<IR::DefaultExpression>()) {
        valueStr = 0x0;
        maskStr = 0x0;
    } else if (k->is<IR::Mask>()) {
        auto km = k->to<IR::Mask>();
        auto value = convertSimpleKeyExpressionToBigInt(km->left, keyWidth, typeMap);
        auto mask = convertSimpleKeyExpressionToBigInt(km->right, keyWidth, typeMap);
        if ((value & mask) != value) {
            ::warning(ErrorType::WARN_MISMATCH,
                      "P4Runtime requires that Ternary matches have masked-off bits set to 0, "
                      "updating value %1% to conform to the P4Runtime specification",
                      km->left);
            value &= mask;
        }
        valueStr = value;
        maskStr = mask;
    } else {
        valueStr = convertSimpleKeyExpressionToBigInt(k, keyWidth, typeMap);
        maskStr = Util::mask(keyWidth);
    }
    print(valueStr, "/");
    print(maskStr, " ");
}

void EmitDpdkTableConfig::addRange(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap) {
    big_int startStr;
    big_int endStr;
    if (k->is<IR::DefaultExpression>()) {
        startStr = 0x0;
        endStr = 0x0;
    } else if (k->is<IR::Range>()) {
        auto kr = k->to<IR::Range>();
        auto start = convertSimpleKeyExpressionToBigInt(kr->left, keyWidth, typeMap);
        auto end = convertSimpleKeyExpressionToBigInt(kr->right, keyWidth, typeMap);
        // Error on invalid range values
        big_int maxValue = (big_int(1) << keyWidth) - 1;
        // NOTE: If end value is > max allowed for keyWidth, value gets
        // wrapped around. A warning is issued in this case by the frontend
        // earlier.
        // For e.g. 16 bit key has a max value of 65535, Range of (1..65536)
        // will be converted to (1..0) and will fail below check.
        if (start > end)
            ::error(ErrorType::ERR_INVALID, "%s Invalid range for table entry", kr->srcInfo);
        startStr = start;
        endStr = end;
    } else {
        startStr = convertSimpleKeyExpressionToBigInt(k, keyWidth, typeMap);
        endStr = startStr;
    }
    print(startStr, "/");
    print(endStr, " ");
}

void EmitDpdkTableConfig::addOptional(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap) {
    if (k->is<IR::DefaultExpression>()) {
        print("0x0/0x0", " ");
    } else {
        auto value = convertSimpleKeyExpressionToBigInt(k, keyWidth, typeMap);
        print(value, " ");
    }
}

void EmitDpdkTableConfig::addMatchKey(const IR::DpdkTable *table, const IR::ListExpression *keyset,
                                      P4::TypeMap *typeMap) {
    int keyIndex = 0;
    for (auto k : keyset->components) {
        auto tableKey = table->getKey()->keyElements.at(keyIndex++);
        auto keyWidth = getTypeWidth(tableKey->expression->type, typeMap);
        auto matchType = getKeyMatchType(tableKey, refMap);
        if (matchType == P4::P4CoreLibrary::instance().exactMatch.name) {
            addExact(k, keyWidth, typeMap);
        } else if (matchType == P4::P4CoreLibrary::instance().lpmMatch.name) {
            addLpm(k, keyWidth, typeMap);
        } else if (matchType == P4::P4CoreLibrary::instance().ternaryMatch.name) {
            addTernary(k, keyWidth, typeMap);
        } else if (matchType == "range") {
            addRange(k, keyWidth, typeMap);
        } else if (matchType == "optional") {
            addOptional(k, keyWidth, typeMap);
        } else {
            if (!k->is<IR::DefaultExpression>())
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "%1%: match type not supported by P4Runtime serializer", matchType);
            continue;
        }
    }
}

/// Checks if the @table entries need to be assigned a priority, i.e. does
/// the match key for the table includes a ternary, range, or optional match?
bool EmitDpdkTableConfig::tableNeedsPriority(const IR::DpdkTable *table, P4::ReferenceMap *refMap) {
    for (auto e : table->getKey()->keyElements) {
        auto matchType = getKeyMatchType(e, refMap);
        // TODO(antonin): remove dependency on v1model.
        if (matchType == "ternary" || matchType == "range" || matchType == "optional") {
            return true;
        }
    }
    return false;
}

bool EmitDpdkTableConfig::isAllKeysDefaultExpression(const IR::ListExpression *keyset) {
    bool allKeyDefaultExp = true;
    for (auto k : keyset->components) {
        allKeyDefaultExp = allKeyDefaultExp && k->is<IR::DefaultExpression>();
    }
    return allKeyDefaultExp;
}

void EmitDpdkTableConfig::postorder(const IR::DpdkTable *table) {
    auto entriesList = table->getEntries();
    if (entriesList == nullptr) return;
    dpdkTableConfigFile.open(table->name + ".txt");
    auto needsPriority = tableNeedsPriority(table, refMap);
    int entryPriority = entriesList->entries.size();
    for (auto e : entriesList->entries) {
        if (!isAllKeysDefaultExpression(e->getKeys())) {
            print("match", " ");
            addMatchKey(table, e->getKeys(), typeMap);
            if (needsPriority) {
                print("priority", " ");
                print(entryPriority--, " ");
            }
            print("action", " ");
            addAction(e->getAction(), refMap, typeMap);
            print("", "\n");
        }
    }
    dpdkTableConfigFile.close();
}
size_t ShortenTokenLength::count = 0;
}  // namespace DPDK
