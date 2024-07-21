/*
Copyright 2024 Nvidia Corp.

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

#ifndef MIDEND_UNROLLLOOPS_H_
#define MIDEND_UNROLLLOOPS_H_

#include "def_use.h"
#include "ir/ir.h"

namespace p4c::P4 {

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

}  // namespace p4c::P4

#endif /* MIDEND_UNROLLLOOPS_H_ */
