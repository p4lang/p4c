/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_P4TEST_MIDEND_H_
#define BACKENDS_P4TEST_MIDEND_H_

#include "frontends/common/options.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"
#include "p4test.h"

namespace P4::P4Test {

class MidEnd : public PassManager {
    std::vector<DebugHook> hooks;

 public:
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    IR::ToplevelBlock *toplevel = nullptr;

    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    explicit MidEnd(P4TestOptions &options, std::ostream *outStream = nullptr);
    IR::ToplevelBlock *process(const IR::P4Program *&program) {
        addDebugHooks(hooks, true);
        program = program->apply(*this);
        return toplevel;
    }
};

}  // namespace P4::P4Test

#endif /* BACKENDS_P4TEST_MIDEND_H_ */
