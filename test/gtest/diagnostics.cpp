/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include <boost/algorithm/string/replace.hpp>
#include <boost/optional.hpp>
#include "gtest/gtest.h"

#include "ir/ir.h"
#include "test/gtest/helpers.h"

namespace Test {

namespace {

boost::optional<FrontendTestCase>
createP4_16DiagnosticsTestCase(const std::string& pragmaSource) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
%PRAGMA_SOURCE%
        struct Headers { bit<8> value; }
        struct Metadata { }
        parser parse(packet_in p, out Headers h, inout Metadata m,
                     inout standard_metadata_t sm) {
            state start { transition accept; } }
        control checksum(inout Headers h, inout Metadata m) { apply { } }
        control mau(inout Headers h, inout Metadata m,
                    inout standard_metadata_t sm) { apply { } }
        control deparse(packet_out p, in Headers h) { apply { } }
        V1Switch(parse(), checksum(), mau(), mau(), checksum(), deparse()) main;
    )");

    boost::replace_first(source, "%PRAGMA_SOURCE%", pragmaSource);

    return FrontendTestCase::create(source);
}

boost::optional<FrontendTestCase>
createP4_14DiagnosticsTestCase(const std::string& pragmaSource) {
    auto source = P4_SOURCE(R"(
%PRAGMA_SOURCE%
        header_type headers_t { fields { value : 8; } }
        header headers_t headers;
        parser start {
            return select(headers.value) { default: ingress; }
        }
        control ingress { }
        control egress { }
    )");

    boost::replace_first(source, "%PRAGMA_SOURCE%", pragmaSource);

    return FrontendTestCase::create(source, CompilerOptions::FrontendVersion::P4_14);
}

}  // namespace

class Diagnostics : public P4CTest { };

TEST_F(Diagnostics, P4_16_Disable) {
    auto test = createP4_16DiagnosticsTestCase(P4_SOURCE(R"(
        @diagnostic("uninitialized_out_param", "disable")
    )"));
    EXPECT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
}

TEST_F(Diagnostics, P4_16_Warn) {
    auto test = createP4_16DiagnosticsTestCase(P4_SOURCE(R"(
        @diagnostic("uninitialized_out_param", "warn")
    )"));
    EXPECT_TRUE(test);
    EXPECT_EQ(1u, ::diagnosticCount());
    EXPECT_EQ(0u, ::errorCount());
}

TEST_F(Diagnostics, P4_16_Error) {
    auto test = createP4_16DiagnosticsTestCase(P4_SOURCE(R"(
        @diagnostic("uninitialized_out_param", "error")
    )"));
    EXPECT_FALSE(test);
    EXPECT_EQ(1u, ::diagnosticCount());
    EXPECT_EQ(1u, ::errorCount());
}

TEST_F(Diagnostics, P4_14_Disable) {
    auto test = createP4_14DiagnosticsTestCase(P4_SOURCE(R"(
        @pragma diagnostic uninitialized_use disable
    )"));
    EXPECT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
}

TEST_F(Diagnostics, P4_14_Warn) {
    auto test = createP4_14DiagnosticsTestCase(P4_SOURCE(R"(
        @pragma diagnostic uninitialized_use warn
    )"));
    EXPECT_TRUE(test);
    EXPECT_EQ(1u, ::diagnosticCount());
    EXPECT_EQ(0u, ::errorCount());
}

TEST_F(Diagnostics, P4_14_Error) {
    auto test = createP4_14DiagnosticsTestCase(P4_SOURCE(R"(
        @pragma diagnostic uninitialized_use error
    )"));
    EXPECT_FALSE(test);
    EXPECT_EQ(1u, ::diagnosticCount());
    EXPECT_EQ(1u, ::errorCount());
}

TEST_F(Diagnostics, NestedCompileContexts) {
    // Check that diagnostic actions set in nested compilation contexts don't
    // affect outer contexts.

    AutoCompileContext autoWarnContext(new GTestContext);
    {
        AutoCompileContext autoDisableContext(new GTestContext);
        {
            AutoCompileContext autoErrorContext(new GTestContext);

            // Run a test with `uninitialized_out_param` set to trigger an error.
            auto test = createP4_16DiagnosticsTestCase(P4_SOURCE(R"(
                @diagnostic("uninitialized_out_param", "error")
            )"));
            EXPECT_FALSE(test);
            EXPECT_EQ(1u, ::diagnosticCount());
            EXPECT_EQ(1u, ::errorCount());
        }

        // Run a test with `uninitialized_out_param` disabled. The error from
        // the previous test should be gone, and no new error should appear.
        auto test = createP4_16DiagnosticsTestCase(P4_SOURCE(R"(
            @diagnostic("uninitialized_out_param", "disable")
        )"));
        EXPECT_TRUE(test);
        EXPECT_EQ(0u, ::diagnosticCount());
    }

    // Run a test with no diagnostic pragma for `uninitialized_out_param`. It
    // should default to triggering a warning; the diagnostic actions configured
    // by the previous tests should be gone.
    auto test = createP4_16DiagnosticsTestCase(P4_SOURCE(R"(
    )"));
    EXPECT_TRUE(test);
    EXPECT_EQ(1u, ::diagnosticCount());
    EXPECT_EQ(0u, ::errorCount());
}

}  // namespace Test
