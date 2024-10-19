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

#ifndef BACKENDS_TOFINO_BF_P4C_TEST_GTEST_TOFINO_GTEST_UTILS_H_
#define BACKENDS_TOFINO_BF_P4C_TEST_GTEST_TOFINO_GTEST_UTILS_H_

#include <optional>
#include <string>

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/device.h"
#include "gtest/gtest.h"
#include "lib/compile_context.h"

namespace P4 {
namespace IR {
namespace BFN {
class Pipe;
}  // namespace BFN
class P4Program;
}  // namespace IR
}  // namespace P4

namespace P4::Test {

const char *tna_header();
const char *t2na_header();

struct MidendTestCase {
    /// Create a test case that requires the frontend and the midend to run.
    static std::optional<MidendTestCase> create(const std::string &source);

    /// The output of the midend.
    const IR::P4Program *program;

    /// The output of the frontend.
    const IR::P4Program *frontendProgram;
};

struct TofinoPipeTestCase {
    /// Create a test case that requires extract_maupipe() to run.
    static std::optional<TofinoPipeTestCase> create(const std::string &source);

    /// Create a test case that requires extract_maupipe() to run, and apply
    /// CreateThreadLocalInstances.
    static std::optional<TofinoPipeTestCase> createWithThreadLocalInstances(
        const std::string &source);

    /// The output of extract_maupipe().
    const IR::BFN::Pipe *pipe;

    /// The output of the frontend.
    const IR::P4Program *frontendProgram;
};

/// A GTest fixture base class for backend targets.
class BackendTest : public ::testing::Test {
 protected:
    BackendTest() : autoBFNContext(new BFNContext()) {}

    AutoCompileContext autoBFNContext;
};

/// A GTest fixture for Tofino tests.
class TofinoBackendTest : public BackendTest {
 public:
    TofinoBackendTest() { Device::init("Tofino"_cs); }
};

/// A GTest fixture for JBay tests.
class JBayBackendTest : public BackendTest {
 public:
    JBayBackendTest() { Device::init("Tofino2"_cs); }
};

}  // namespace P4::Test

#endif /* BACKENDS_TOFINO_BF_P4C_TEST_GTEST_TOFINO_GTEST_UTILS_H_ */
