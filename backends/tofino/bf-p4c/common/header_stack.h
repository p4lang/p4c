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

#ifndef BF_P4C_COMMON_HEADER_STACK_H_
#define BF_P4C_COMMON_HEADER_STACK_H_

#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/map.h"

using namespace P4;

class PhvInfo;

/** Walk over the IR and collect metadata about the usage of header stacks in
 * the program. The results are stored in `IR::BFN::Pipe::headerStackInfo`.
 *
 * @warning Cannot run after InstructionSelection, which replaces push and
 *          pop operations with `set` instructions.
 */
struct CollectHeaderStackInfo : public Modifier {
    CollectHeaderStackInfo();

 private:
    Visitor::profile_t init_apply(const IR::Node *root) override;
    void postorder(IR::HeaderStack *stack) override;
    void postorder(IR::MAU::Primitive *primitive) override;
    void postorder(IR::BFN::Pipe *pipe) override;

    BFN::HeaderStackInfo *stacks;
};

/// Remove header stack info for unused headers, eg. removed by ElimUnused.  Can be used after
/// InstructionSelection, unlike CollectHeaderStackInfo.
class ElimUnusedHeaderStackInfo : public PassManager {
    ordered_set<cstring> unused;

    // Find unused headers.
    struct Find : public Inspector {
        ElimUnusedHeaderStackInfo &self;
        const BFN::HeaderStackInfo *stacks = nullptr;
        ordered_set<cstring> used;

        explicit Find(ElimUnusedHeaderStackInfo &self) : self(self) {}

        Visitor::profile_t init_apply(const IR::Node *root) override {
            self.unused.clear();
            used.clear();
            return Inspector::init_apply(root);
        }

        bool preorder(const IR::BFN::Pipe *pipe) override {
            BUG_CHECK(pipe->headerStackInfo != nullptr,
                      "Running ElimUnusedHeaderStackInfo without running "
                      "CollectHeaderStackInfo first?");
            stacks = pipe->headerStackInfo;
            return true;
        }

        void postorder(const IR::HeaderStack *stack) override;
        void end_apply() override;
    };

    // Remove from pipe->headerStackInfo.
    struct Elim : public Modifier {
        ElimUnusedHeaderStackInfo &self;
        explicit Elim(ElimUnusedHeaderStackInfo &self) : self(self) {}

        void postorder(IR::BFN::Pipe *pipe) override;
    };

 public:
    ElimUnusedHeaderStackInfo() { addPasses({new Find(*this), new Elim(*this)}); }
};

namespace BFN {

/// Metadata about how header stacks are used in the program.
struct HeaderStackInfo {
    struct Info {
        /// The name of the header stack this metadata describes.
        cstring name;

        /// Which threads is this header stack visible in? By default, the same
        /// header stack is used in both threads, but after
        /// CreateThreadLocalInstances runs, header stacks are thread-specific.
        bool inThread[2] = {true, true};

        /// How many elements are in this header stack?
        int size = 0;

        /// What is the maximum number of elements that are pushed onto this
        /// header stack in a single `push_front` primitive invocation?
        int maxpush = 0;

        /// What is the maximum number of elements that are popped off of this
        /// header stack in a single `pop_front` primitive invocation?
        int maxpop = 0;
    };

 private:
    friend struct ::CollectHeaderStackInfo;
    friend class ::ElimUnusedHeaderStackInfo;
    ordered_map<cstring, Info> info;

 public:
    auto begin() const -> decltype(Values(info).begin()) { return Values(info).begin(); }
    auto begin() -> decltype(Values(info).begin()) { return Values(info).begin(); }
    auto end() const -> decltype(Values(info).end()) { return Values(info).end(); }
    auto end() -> decltype(Values(info).end()) { return Values(info).end(); }
    auto at(cstring n) const -> decltype(info.at(n)) { return info.at(n); }
    auto at(cstring n) -> decltype(info.at(n)) { return info.at(n); }
    auto count(cstring n) const -> decltype(info.count(n)) { return info.count(n); }
    const Info *get(cstring n) const {
        if (auto it = info.find(n); it != info.end()) return &it->second;
        return nullptr;
    }
};
}  // namespace BFN

/** Remove setValid when used to initialize a newly-pushed element of a header
 * stack, and fail with P4C_UNIMPLEMENTED if no such initialization occurs in
 * the same action as the push.
 *
 * This is because P4_16 defines push_front as pushing an invalid header, but
 * P4_14 (and our current implementation) implements push_front as pushing a
 * *valid* header.
 *
 * @pre Must run before ValidToStkvalid, because this pass refers to the
 * "$valid" suffix of header stack elements, and before HeaderPushPop, because
 * it looks for "push_front" instructions.
 */
class RemovePushInitialization : public Transform {
    IR::Node *preorder(IR::MAU::Action *act) override;

 public:
    RemovePushInitialization() {}
};

/** Replace the "$valid" suffix for header stacks with an AliasSlice index into
 * the "$stkvalid" field instead.  Also updates the alias map in PhvInfo.
 *
 * @pre Must happen after ResolveComputedParserExpressions.
 */
class ValidToStkvalid : public Transform {
    struct BFN::HeaderStackInfo *stack_info_ = nullptr;

    // Populate stack_info_.
    IR::Node *preorder(IR::BFN::Pipe *pipe) override;

    // Replace uses of stk[x].$valid with stk.$stkvalid[y:y], where the latter
    // corresponds to the validity bit in $stkvalid of element x of stk.
    IR::Node *postorder(IR::Member *member) override;

    // Replace extractions to slices of $stkvalid with equivalent extractions
    // to the entire field.  Eg. extract(stk.$stkvalid, 0x4) instead of
    // extract(stk.$stkvalid[2:2], 0x1).
    //
    // TODO: This is because Brig doesn't currently support extracting
    // constants to field slices in the parser (see BRIG-584).  As a result,
    // this removes the AliasSlice node and hence loses the aliasing
    // information.  However, as parser extracts aren't exposed to the control
    // plane, this should be fine.
    IR::Node *postorder(IR::BFN::Extract *extract) override;

 public:
    explicit ValidToStkvalid(PhvInfo &) {}
};

#endif /* BF_P4C_COMMON_HEADER_STACK_H_ */
