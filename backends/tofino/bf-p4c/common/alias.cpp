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

#include <sstream>
#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/common/alias.h"
#include "bf-p4c/common/ir_utils.h"

Visitor::profile_t FindExpressionsForFields::init_apply(const IR::Node* root) {
    profile_t rv = Inspector::init_apply(root);
    fieldNameToExpressionsMap.clear();
    return rv;
}

bool FindExpressionsForFields::preorder(const IR::HeaderOrMetadata* h) {
    LOG5("Header: " << h->name);
    LOG5("Header type: " << h->type);
    for (auto f : h->type->fields) {
        if (auto hs = h->to<IR::HeaderStack>()) {
            for (int i = 0; i < hs->size; i++) {
                // add every header in header stack
                cstring name = h->name + "[" + cstring::to_cstring(i)  + "]." + f->name;
                if (name.endsWith("$always_deparse") ||
                    name.endsWith(BFN::BRIDGED_MD_INDICATOR.string_view()))
                    continue;
                const IR::Type *ftype = nullptr;
                auto field = hs->type->getField(f->name);
                if (field != nullptr)
                    ftype = field->type;
                else
                    BUG("Couldn't find header stack field %s in %s", f->name, hs->name);
                IR::Member* mem =
                    new IR::Member(
                        ftype,
                        new IR::HeaderStackItemRef(
                            new IR::ConcreteHeaderRef(h), new IR::Constant(i)),
                        f->name);
                fieldNameToExpressionsMap[name] = mem;
                LOG5("  Added field: " << name << ", " << mem);
            }
        } else {
            cstring name = h->name + "."_cs + f->name;
            // Ignore compiler generated metadata fields that belong to different headers.
            if (name.endsWith("$always_deparse") ||
                name.endsWith(BFN::BRIDGED_MD_INDICATOR.string_view()))
                continue;
            IR::Member* mem = gen_fieldref(h, f->name);
            if (!mem) continue;
            fieldNameToExpressionsMap[name] = mem;
            LOG5("  Added field: " << name << ", " << mem);
        }
    }
    if (auto hs = h->type->to<IR::HeaderStack>()) {
        for (int i = 0; i < hs->size; i++) {
            cstring name = h->name + "[" + cstring::to_cstring(i)  + "].$valid";
            IR::Member* mem = new IR::Member(
                IR::Type_Bits::get(1),
                new IR::HeaderStackItemRef(new IR::ConcreteHeaderRef(h), new IR::Constant(i)),
                "$valid");
            if (mem) {
                fieldNameToExpressionsMap[name] = mem;
                LOG5("  Added field: " << name << ", " << mem);
            }
        }
    } else if (h->type->is<IR::Type_Header>()) {
        cstring name = h->name + ".$valid"_cs;
        IR::Member* mem = new IR::Member(IR::Type_Bits::get(1),
                new IR::ConcreteHeaderRef(h), "$valid");
        if (mem) {
            fieldNameToExpressionsMap[name] = mem;
            LOG5("  Added field: " << name << ", " << mem);
        }
    }
    return true;
}

Visitor::profile_t ReplaceAllAliases::init_apply(const IR::Node* root) {
    if (LOGGING(3)) {
        LOG1("    All aliasing fields: ");
        for (auto &kv : pragmaAlias.getAliasMap()) {
            std::stringstream ss;
            if (kv.second.range)
                ss << "[" << kv.second.range->hi << ":" << kv.second.range->lo << "]";
            LOG1("      " << kv.first << " -> " << kv.second.field << ss.str()); } }
    return Transform::init_apply(root);
}

// make hw_constrained_fields visible to this Transform
IR::Node* ReplaceAllAliases::preorder(IR::BFN::Pipe* pipe) {
    visit(pipe->thread[INGRESS].hw_constrained_fields);
    visit(pipe->thread[EGRESS].hw_constrained_fields);
    return pipe;
}

IR::Node* ReplaceAllAliases::preorder(IR::Expression* expr) {
    auto* f = phv.field(expr);
    if (!f) {
        LOG4("    Field not found for expression: " << expr);
        return expr; }

    // Determine alias source fields
    auto& aliasMap = pragmaAlias.getAliasMap();
    if (!aliasMap.count(f->name)) {
        LOG4("    Field " << f->name << " not part of any aliasing.");
        return expr; }
    auto* ixbarread = expr->to<IR::MAU::TableKey>();
    if (ixbarread) {
        LOG4("    Expression " << expr << " is an input xbar read.");
        return expr; }

    // replacementField is the alias destination field
    auto destInfo = aliasMap.at(f->name);
    const PHV::Field* replacementField = phv.field(destInfo.field);
    CHECK_NULL(replacementField);
    // If this is a HardwareConstraintField and the target doesn't exist; don't make any change.
    if (getParent<IR::BFN::HardwareConstrainedField>() &&
        !fieldExpressions.count(replacementField->name))
        return expr;
    // FIXME -- if the target doesn't exist, this is *probably* a HardwareConstrainedField that
    // didn't get identified as such.  We should figure out why that happened and fix it, but
    // if we just ignore it things should work out ok, if sub-optimally.
    if (!fieldExpressions.count(replacementField->name)) return expr;
    // BUG_CHECK(fieldExpressions.count(replacementField->name),
    //         "Expression not found %1%", replacementField->name);

    // replacementExpression is the expression corresponding to the alias destination field
    const IR::Member* replacementMember = fieldExpressions.at(replacementField->name);

    auto* sl = expr->to<IR::Slice>();
    if (sl) {
        if (f->size == replacementField->size) {
            auto* newMember = new IR::BFN::AliasMember(replacementMember, sl->e0);
            IR::Slice* newSlice = new IR::Slice(newMember, sl->getH(), sl->getL());
            LOG2("    Replaced A: " << expr << " -> " << newSlice);
            return newSlice;
        } else {
            IR::Slice* destSlice = new IR::Slice(replacementMember, sl->getH(), sl->getL());
            auto* newAliasSlice = new IR::BFN::AliasSlice(destSlice, sl);
            LOG2("    Replaced B: " << expr << " -> " << newAliasSlice);
            return newAliasSlice;
        }
    } else {
        if (f->size == replacementField->size) {
            auto* newMember = new IR::BFN::AliasMember(replacementMember, expr);
            LOG2("    Replaced C: " << expr << " -> " << newMember);
            return newMember;
        } else {
            IR::Slice* destSlice = new IR::Slice(replacementMember, f->size - 1, 0);
            auto* newAliasSlice = new IR::BFN::AliasSlice(destSlice, expr);
            LOG2("    Replaced D: " << expr << " -> " << newAliasSlice);
            return newAliasSlice;
        }
    }
}

inline cstring AddValidityBitSets::getFieldValidityName(cstring origName) {
    return origName + ".$valid";
}

Visitor::profile_t AddValidityBitSets::init_apply(const IR::Node* root) {
    aliasedFields.clear();
    actionToPOVMap.clear();
    for (auto& kv : pragmaAlias.getAliasMap()) {
        const cstring validFieldName = getFieldValidityName(kv.first);
        const PHV::Field* field = phv.field(validFieldName);
        if (!field) continue;
        aliasedFields[phv.field(kv.second.field)].insert(field);
        LOG1("\tAliased field " << kv.second.field << " has a validity bit from "
             << kv.first << " (" << field->name << ")");
    }
    return Transform::init_apply(root);
}

IR::Node* AddValidityBitSets::preorder(IR::MAU::Instruction* inst) {
    if (inst->operands.size() == 0) return inst;
    const PHV::Field* f = phv.field(inst->operands.at(0));
    if (!aliasedFields.count(f)) return inst;
    auto* action = findContext<IR::MAU::Action>();
    if (!action) return inst;
    for (const auto* pov : aliasedFields.at(f))
        actionToPOVMap[action->name].insert(pov);
    return inst;
}

IR::Node* AddValidityBitSets::postorder(IR::MAU::Action* action) {
    if (!actionToPOVMap.count(action->name)) return action;
    for (const auto* f : actionToPOVMap.at(action->name)) {
        auto* pov = phv.getTempVar(f);
        if (!pov) {
            LOG3("\t  Could not find POV corresponding to " << f->name);
            continue;
        }
        auto* oneExpr = new IR::Constant(IR::Type_Bits::get(pov->type->width_bits()), 1);
        auto* prim = new IR::MAU::Instruction("set"_cs, { pov, oneExpr });
        LOG1("\t  Add to action " << action->name << " : " << prim);
        action->action.push_back(prim);
    }
    return action;
}
