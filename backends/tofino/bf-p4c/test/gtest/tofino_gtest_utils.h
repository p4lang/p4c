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
