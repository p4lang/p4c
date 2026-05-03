/*
 * Copyright 2024 Intel Corp.
 * SPDX-FileCopyrightText: 2024 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_GTEST_MIDEND_PASS_H_
#define TEST_GTEST_MIDEND_PASS_H_

#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "ir/ir.h"
#include "midend/actionSynthesis.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
class ComputeDefUse;
class ToplevelBlock;
}  // namespace P4

namespace P4 {
class CompilerOptions;
}  // namespace P4

namespace P4::Test {

class SkipControls : public P4::ActionSynthesisPolicy {
    const std::set<cstring> *skip;

 public:
    explicit SkipControls(const std::set<cstring> *skip) : skip(skip) { CHECK_NULL(skip); }
    bool convert(const Visitor::Context *, const IR::P4Control *control) override {
        if (skip->find(control->name) != skip->end()) return false;
        return true;
    }
};

/// Collection of midend passes for tests to use
class MidEnd : public PassManager {
 public:
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    P4::ComputeDefUse *defuse;
    IR::ToplevelBlock *toplevel = nullptr;

    explicit MidEnd(CompilerOptions &options, std::ostream *outStream = nullptr);
    IR::ToplevelBlock *process(const IR::P4Program *&program);
};

}  // namespace P4::Test

#endif /* TEST_GTEST_MIDEND_PASS_H_ */
