/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "header_stack.h"

#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/phv/phv_fields.h"

using namespace P4;

CollectHeaderStackInfo::CollectHeaderStackInfo() { stacks = new BFN::HeaderStackInfo; }

Visitor::profile_t CollectHeaderStackInfo::init_apply(const IR::Node *root) {
    auto rv = Modifier::init_apply(root);
    stacks->info.clear();
    LOG3("Begin CollectHeaderStackInfo");
    return rv;
}

void CollectHeaderStackInfo::postorder(IR::HeaderStack *hs) {
    if (stacks->info.count(hs->name)) return;

    auto &i = stacks->info[hs->name];
    LOG4("Adding header stack " << cstring(hs->name));

    i.name = hs->name;
    i.size = hs->size;
    i.maxpush = i.maxpop = 0;

    // By default, header stacks are visible in both threads, but if we've
    // already run CreateThreadLocalInstances then this header stack will only
    // be visible in a single thread.
    if (i.name.startsWith("ingress::"))
        i.inThread[EGRESS] = false;
    else if (i.name.startsWith("egress::"))
        i.inThread[INGRESS] = false;
}

void CollectHeaderStackInfo::postorder(IR::MAU::Primitive *prim) {
    if (prim->name == "push_front" || prim->name == "pop_front") {
        LOG3("CollectHeaderStackInfo: visiting " << prim);
        BUG_CHECK(prim->operands.size() == 2, "wrong number of operands to %s", prim);
        cstring hsname = prim->operands[0]->toString();
        if (!stacks->info.count(hsname)) {
            /* Should have been caught by typechecking? */
            error("%s: No header stack %s", prim->srcInfo, prim->operands[0]);
            return;
        }
        int &max =
            (prim->name == "push_front") ? stacks->at(hsname).maxpush : stacks->at(hsname).maxpop;
        if (auto count = prim->operands[1]->to<IR::Constant>()) {
            auto countval = count->asInt();
            if (countval <= 0) {
                error("%s: %s amount must be > 0", count->srcInfo, prim->name);
            } else if (countval > max) {
                max = countval;
                LOG3("CollectHeaderStackInfo: ...setting max to " << max);
            } else {
                LOG3("CollectHeaderStackInfo: ...max " << max << " >= " << countval);
            }
        } else {
            error("%s: %s amount must be constant", prim->operands[1]->srcInfo, prim->name);
        }
        LOG3("CollectHeaderStackInfo: ...maxpush: " << stacks->at(hsname).maxpush);
        LOG3("CollectHeaderStackInfo: ...maxpop: " << stacks->at(hsname).maxpop);
        // Get maxpush and maxpop after push and pop is converted in to modify/set
    } else if (prim->name == "modify_field" || prim->name == "set") {
        auto operand0 = prim->operands[0]->to<IR::Slice>();
        auto operand1 = prim->operands[1]->to<IR::Slice>();
        if (!operand0 || !operand1) return;
        auto op0 = operand0->e0->to<IR::Member>();
        auto op1 = operand1->e0->to<IR::Member>();
        if (!op0 || !op1) return;
        if (op0->member == "$stkvalid" && op0->expr->type->is<IR::Type_Stack>() &&
            op1->member == "$stkvalid" && op1->expr->type->is<IR::Type_Stack>()) {
            auto &s = stacks->at(op0->expr->toString());
            // Because of the way stkvalid is formatted, the lower limit of the
            // first the operand will always be equal to maxpop
            if (auto e2 = operand0->e2->to<IR::Constant>()) {
                int total_size = op0->type->width_bits();
                s.maxpop = e2->asInt();
                s.maxpush = total_size - s.maxpop - s.size;
                LOG3("CollectHeaderStackInfo: ...MAXPUSH: "
                     << stacks->at(op0->expr->toString()).maxpush);
                LOG3("CollectHeaderStackInfo: ...MAXPOP: "
                     << stacks->at(op0->expr->toString()).maxpop);
            }
        }
    }
}

void CollectHeaderStackInfo::postorder(IR::BFN::Pipe *pipe) {
    // Store the information we've collected in
    // `IR::BFN::Pipe::headerStackInfo` so other passes can access it.
    pipe->headerStackInfo = stacks;
    if (LOGGING(3)) {
        LOG3("CollectHeaderStackInfo: Collected");
        for (auto kv : stacks->info) {
            LOG3("  " << kv.first << ": " << kv.second.name << " (" << kv.second.maxpush << " / "
                      << kv.second.maxpop << ")");
        }
    }
}

void ElimUnusedHeaderStackInfo::Find::postorder(const IR::HeaderStack *hs) {
    used.insert(hs->name);
}

void ElimUnusedHeaderStackInfo::Find::end_apply() {
    for (auto &hs : *stacks) {
        if (used.find(hs.name) == used.end()) {
            LOG5("ElimUnusedHeaderStackInfo: Found unused stack " << hs.name);
            self.unused.insert(hs.name);
        } else {
            LOG5("ElimUnusedHeaderStackInfo: Found used stack " << hs.name);
        }
    }
}

void ElimUnusedHeaderStackInfo::Elim::postorder(IR::BFN::Pipe *pipe) {
    for (auto &hs : self.unused) {
        LOG5("ElimUnusedHeaderStackInfo: Removing stack " << hs);
        pipe->headerStackInfo->info.erase(hs);
    }
}

IR::Node *RemovePushInitialization::preorder(IR::MAU::Action *act) {
    // Maps the header to a bitmap of the indicies pushed so far.
    ordered_map<cstring, bitvec> pushed;

    // Maps the header to a bitmap of the indicies pushed so far, which are
    // then cleared when an initialization is found.
    ordered_map<cstring, bitvec> pushed_inits;

    ordered_map<cstring, const IR::MAU::Primitive *> push_prims;
    IR::Vector<IR::MAU::Primitive> to_keep;

    for (auto *prim : act->action) {
        to_keep.push_back(prim);
        if (prim->name == "push_front") {
            // If this is the first push to this field in this action, add it
            // to pushed.  Otherwise, shift the bitvec to account for
            // the newly pushed elements.
            cstring dst = prim->operands[0]->toString();
            int pushAmount = prim->operands[1]->to<IR::Constant>()->asInt();
            LOG5("Found " << dst << " with " << pushAmount
                          << " pushed elements that need to be initialized: " << prim);
            if (pushed.find(dst) == pushed.end()) {
                pushed[dst] = bitvec(0, pushAmount);
                pushed_inits[dst] = bitvec(0, pushAmount);
            } else {
                pushed[dst] <<= pushAmount;
                pushed[dst].setrange(0, pushAmount);
                pushed_inits[dst] <<= pushAmount;
                pushed_inits[dst].setrange(0, pushAmount);
                push_prims[dst] = prim;
            }
        } else if (prim->name == "modify_field") {
            // If this sets the validity of a header stack element, check that
            // it's a newly-added element, and mark it for removal.
            if (auto *dst = prim->operands[0]->to<IR::Member>()) {
                if (dst->member.toString() != "$valid") continue;
                if (auto *ref = dst->expr->to<IR::HeaderStackItemRef>()) {
                    if (auto *idxNode = ref->index()->to<IR::Constant>()) {
                        if (auto *src = prim->operands[1]->to<IR::Constant>()) {
                            if (src->asInt() != 1) continue;
                            auto stkName = ref->base()->toString();
                            if (pushed.find(stkName) == pushed.end()) continue;
                            int idx = idxNode->asInt();
                            auto &inits = pushed.at(stkName);
                            if (inits.getbit(idx)) {
                                to_keep.pop_back();
                                pushed_inits[stkName].clrbit(idx);
                            }
                        }
                    }
                }
            }
        }
    }

    for (auto &kv : pushed_inits) {
        if (kv.second != bitvec()) {
            std::stringstream msg;
            for (auto idx : kv.second) msg << idx << " ";
            // TODO: This should really be P4C_UNIMPLEMENTED.
            warning(
                "Header stack elements must be initialized in the action in which "
                "they are pushed.  %1% is pushed but indices %3%are not "
                "initialized in action %2%",
                kv.first, cstring::to_cstring(act), msg.str());
        }
    }

    // Remove setValid of pushed elements; validity is set automatically as
    // part of the push operation.
    act->action = to_keep;
    LOG4("New action with validity initialization removed: " << act);
    return act;
}

IR::Node *ValidToStkvalid::preorder(IR::BFN::Pipe *pipe) {
    LOG5("ENTERING ValidToStkvalid");
    stack_info_ = pipe->headerStackInfo;
    BUG_CHECK(stack_info_,
              "No header stack info.  Running ValidToStkvalid "
              "before CollectHeaderStackInfo?");
    return pipe;
}

IR::Node *ValidToStkvalid::postorder(IR::Member *member) {
    LOG5("\tmember: " << member);
    auto *ref = member->expr->to<IR::HeaderStackItemRef>();
    // Skip everything but header stack validity fields (hdr[x].$valid).
    if (!ref || member->member.toString() != "$valid") return member;

    auto *stk = ref->baseRef();
    if (!stack_info_->count(stk->name.toString())) return member;
    auto info = stack_info_->at(stk->name.toString());

    auto *idxNode = ref->index()->to<IR::Constant>();
    BUG_CHECK(idxNode,
              "Found a header stack with a non-constant reference in "
              "the back end: %1%",
              cstring::to_cstring(member));
    int idx = idxNode->asInt();

    // Translate the idx to the bit position in $stkvalid corresponding to this
    // element's validity bit.  See the comment for HeaderPushPop for details
    // on the stkvalid encoding.
    int stkvalidIdx = info.maxpop + info.size - 1 - idx;
    BUG_CHECK(stkvalidIdx >= 0 && stkvalidIdx < info.size + info.maxpop + info.maxpush,
              "stkvalidIdx %d out of range for %s", stkvalidIdx, ref->base());

    auto *stkvalid =
        new IR::Member(member->srcInfo, IR::Type::Bits::get(info.size + info.maxpop + info.maxpush),
                       ref->base(), "$stkvalid");
    auto *slice = new IR::Slice(stkvalid, stkvalidIdx, stkvalidIdx);
    auto *alias = new IR::BFN::AliasSlice(slice, member);

    LOG5("Replacing " << member << " with " << alias);
    return alias;
}

IR::Node *ValidToStkvalid::postorder(IR::BFN::Extract *extract) {
    LOG5("\textract: " << extract);
    // Check whether the destination is an extraction to a $stkvalid slice.  If
    // so, get the bit being written to.
    auto *fieldLVal = extract->dest->to<IR::BFN::FieldLVal>();
    if (!fieldLVal) return extract;
    auto *slice = fieldLVal->field->to<IR::Slice>();
    if (!slice) return extract;
    auto *member = slice->e0->to<IR::Member>();
    if (!member || member->member.toString() != "$stkvalid") return extract;
    auto *src = extract->source->to<IR::BFN::ConstantRVal>();
    if (!src || src->constant->asInt() != 1) return extract;
    int dst = slice->getL();

    // Replace the slice with $stkvalid, and shift the value being assigned to
    // the offset of the destination bit.
    //
    // TODO: This discards aliasing information, because it replaces the
    // stkvalid slice (which is aliased to $valid) with the entirety of
    // $stkvalid, which is not.
    extract->dest = new IR::BFN::FieldLVal(member);
    extract->source = new P4::IR::BFN::ConstantRVal(1 << dst);
    return extract;
}
