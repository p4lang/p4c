/*
 * Copyright 2024 Nvidia Corp.
 * SPDX-FileCopyrightText: 2024 Nvidia Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_UNROLLLOOPS_H_
#define MIDEND_UNROLLLOOPS_H_

#include "def_use.h"
#include "ir/ir.h"

namespace P4 {

class UnrollLoops : public Transform, public P4::ResolutionContext {
    NameGenerator &nameGen;
    const ComputeDefUse *defUse;

 public:
    struct loop_bounds_t {
        const IR::Declaration_Variable *index = nullptr;
        std::vector<long> indexes;
    };
    struct Policy {
        bool unroll_default;
        virtual bool operator()(const IR::LoopStatement *, bool, const loop_bounds_t &);
        explicit Policy(bool ud) : unroll_default(ud) {}
    } & policy;
    static Policy default_unroll, default_nounroll;

 private:
    long evalLoop(const IR::Expression *, long, const ComputeDefUse::locset_t &, bool &);
    long evalLoop(const IR::BaseAssignmentStatement *, long, const ComputeDefUse::locset_t &,
                  bool &);
    bool findLoopBounds(IR::ForStatement *, loop_bounds_t &);
    bool findLoopBounds(IR::ForInStatement *, loop_bounds_t &);
    const IR::Statement *doUnroll(const loop_bounds_t &, const IR::Statement *,
                                  const IR::IndexedVector<IR::StatOrDecl> * = nullptr);

    const IR::Statement *preorder(IR::ForStatement *) override;
    const IR::Statement *preorder(IR::ForInStatement *) override;

 public:
    explicit UnrollLoops(NameGenerator &ng, const ComputeDefUse *du, Policy &p = default_unroll)
        : nameGen(ng), defUse(du), policy(p) {}
};

}  // namespace P4

#endif /* MIDEND_UNROLLLOOPS_H_ */
