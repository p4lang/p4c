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

#include "bf-p4c/phv/create_thread_local_instances.h"

#include "bf-p4c/ir/gress.h"
#include "bf-p4c/ir/thread_visitor.h"
#include "bf-p4c/mau/mau_visitor.h"

namespace {

using ThreadLocalMetadataMapping =
    std::map<const IR::HeaderOrMetadata *, const IR::HeaderOrMetadata *>;

class FindSharedStateful : public MauInspector {
    struct SaluThreadUse {
        std::set<gress_t> threads;                 // which threads use this salu
        std::map<cstring, gress_t> action_thread;  // which threads use each stateful action
    };
    std::map<const IR::MAU::StatefulAlu *, SaluThreadUse> all_salus;

    // This runs before InstructionSelection, so SALU refs are still RegisterAction.execute
    // primitives within actions, and not in IR::MAU::Action::stateful
    bool preorder(const IR::MAU::StatefulAlu *salu) {
        visitAgain();
        gress_t thread = VisitingThread(this);
        all_salus[salu].threads.insert(thread);
        LOG2("FindSharedStateful Adding salu " << salu->name << " on thread : " << thread);
        if (auto *tbl = findContext<IR::MAU::Table>()) {
            if (auto *act = findContext<IR::MAU::Action>()) {
                if (auto *sact = salu->calledAction(tbl, act)) {
                    all_salus.at(salu).action_thread[sact->name] = thread;
                    LOG3("  Adding salu action " << sact->name << "on thread : " << thread);
                }
            }
        }
        return false;
    }
    bool preorder(const IR::GlobalRef *) {
        visitAgain();
        return true;
    }

 public:
    std::pair<bool, gress_t> salu_action_gress(const IR::MAU::StatefulAlu *salu, cstring action) {
        if (all_salus.count(salu)) {
            const auto salu_action_threads = all_salus.at(salu).action_thread;
            if (salu_action_threads.count(action)) {
                return std::make_pair(true, salu_action_threads.at(action));
            }
        }
        return std::make_pair(false, gress_t());
    }
};

/// Create thread-local versions of variables and parser states.
struct CreateLocalInstances : public Transform {
    FindSharedStateful *salus;
    explicit CreateLocalInstances(FindSharedStateful *salus) : salus(salus) {}

    /// The thread-local versions of each header and metadata struct; consumed by
    /// CreateThreadLocalMetadata.
    ThreadLocalMetadataMapping localMetadata[3];
    std::map<const IR::TempVar *, const IR::TempVar *> localTempVars[3];
    std::map<const IR::Padding *, const IR::Padding *> localPaddings[3];
    std::map<const IR::Expression *, std::set<const IR::Expression *>> memoizeExpr;

 private:
    const IR::HeaderOrMetadata *ghost_intrinsic_metadata = nullptr;
    profile_t init_apply(const IR::Node *root) override {
        ghost_intrinsic_metadata = nullptr;
        return Transform::init_apply(root);
    }

    gress_t get_thread() const {
        if (auto *salu_action = findContext<IR::MAU::SaluAction>()) {
            auto *salu = findOrigCtxt<IR::MAU::StatefulAlu>();
            auto action_gress = salus->salu_action_gress(salu, salu_action->name);
            if (action_gress.first) return action_gress.second;
        }
        return VisitingThread(this);
    }

    /// Prepend "thread-name::" to the names of headers and metadata structs.
    const IR::HeaderOrMetadata *preorder(IR::HeaderOrMetadata *header) override {
        LOG1("CreateLocalInstances preorder Header / Metadata: " << header);
        const auto *rv = header;
        auto *orig = getOriginal<IR::HeaderOrMetadata>();
        visitAgain();
        gress_t thread;
        if (header->type->annotations->getSingle("__ghost_metadata"_cs)) {
            // ghost intrinsic metadata is declared separately as arguments to both
            // ingress and ghost, but both need to refer to the same thing (same PHV)
            thread = GHOST;
            if (ghost_intrinsic_metadata)
                rv = ghost_intrinsic_metadata;
            else
                ghost_intrinsic_metadata = header;
        } else {
            thread = get_thread();
        }
        if (localMetadata[thread].count(orig)) {
            rv = localMetadata[thread][orig];
        } else {
            if (rv == header) header->name = IR::ID(createThreadName(thread, header->name));
            localMetadata[thread][orig] = rv;
        }
        prune();
        return rv;
    }
    /// Prepend "thread-name::" to TempVars, exactly once per thread
    const IR::TempVar *preorder(IR::TempVar *var) override {
        LOG1("CreateLocalInstances preorder Tempvar : " << var);
        auto *orig = getOriginal<IR::TempVar>();
        visitAgain();
        gress_t thread = get_thread();
        if (localTempVars[thread].count(orig)) {
            return localTempVars[thread][orig];
        }
        var->name = createThreadName(thread, var->name);
        localTempVars[thread][orig] = var;
        return var;
    }

    /// Prepend "thread-name::" to Paddings, exactly once per thread
    const IR::Padding *preorder(IR::Padding *var) override {
        LOG1("CreateLocalInstances preorder padding : " << var);
        auto *orig = getOriginal<IR::Padding>();
        visitAgain();
        gress_t thread = get_thread();
        if (localPaddings[thread].count(orig)) {
            return localPaddings[thread][orig];
        }
        var->name = createThreadName(thread, var->name);
        localPaddings[thread][orig] = var;
        return var;
    }

    const IR::Expression *preorder(IR::Expression *hr) override {
        // Need to ensure that any Expression that has a subexpression HeaderOrMetadata or
        // TempVar that got split based on thread is also split...
        // is different
        visitAgain();
        return hr;
    }
    const IR::Expression *postorder(IR::Expression *hr) override {
        LOG1("CreateLocalInstances postorder expression : " << hr);
        // but only if it is different from any previously created clone.
        auto *orig = getOriginal<IR::Expression>();
        for (auto *gen : memoizeExpr[orig])
            if (*gen == *hr) return gen;
        memoizeExpr[orig].insert(hr);
        return hr;
    }
};

/// Update the set of metadata structs in `BFN::Pipe::metadata` to refer to
/// the thread-local versions of the structs.
struct CreateThreadLocalMetadata : public Modifier {
    explicit CreateThreadLocalMetadata(const ThreadLocalMetadataMapping localMetadata[3])
        : localMetadata(localMetadata) {}

 private:
    bool preorder(IR::BFN::Pipe *pipe) override {
        // Replace the global metadata variables in the program with
        // thread-local versions. There are at most two thread-local versions of
        // each variable; there may be fewer than two if the variable is only
        // used by one thread.
        IR::NameMap<IR::HeaderOrMetadata> instancedMetadata;
        for (auto &item : pipe->metadata) {
            cstring name = item.first;
            auto *metadata = item.second;
            for (auto gress : {INGRESS, EGRESS, GHOST}) {
                auto gressName = createThreadName(gress, name);
                auto &gressMetadata = localMetadata[gress];
                if (gressMetadata.find(metadata) != gressMetadata.end()) {
                    instancedMetadata.addUnique(gressName, gressMetadata.at(metadata));
                }
            }
        }

        pipe->metadata = instancedMetadata;
        return false;
    }

    const ThreadLocalMetadataMapping *localMetadata;
};

}  // namespace

CreateThreadLocalInstances::CreateThreadLocalInstances() {
    auto *saluThread = new FindSharedStateful;
    auto *createLocalInstances = new CreateLocalInstances(saluThread);
    addPasses({saluThread, createLocalInstances,
               new CreateThreadLocalMetadata(createLocalInstances->localMetadata)});
}
