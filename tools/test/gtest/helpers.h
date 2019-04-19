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

#ifndef TEST_GTEST_HELPERS_H_
#define TEST_GTEST_HELPERS_H_

#include <boost/optional.hpp>
#include <string>

#include "frontends/common/options.h"
#include "frontends/p4/parseAnnotations.h"
#include "gtest/gtest.h"

namespace IR {
class P4Program;
}  // namespace IR

/// Specifies which standard headers should be included by a GTest.
enum class P4Headers {
    NONE,    // No headers.
    CORE,    // Just core.p4.
    V1MODEL,  // Both core.p4 and v1model.p4.
    PSA      // Both core.p4 and psa.p4
};

namespace detail {

/**
 * Transforms the P4 program (or program fragment) in @rawSource to turn it into
 * a complete program and make it more suitable for debugging.
 *
 * @param file  The __FILE__ at which @rawSource is defined; used along with
 *              @line to improve compiler error messages.
 * @param line  The __LINE__ at which @rawSource is defined.
 * @param headers  Specifies any standard headers that should be prepended to
 *                 @rawSource to make a complete P4 program.
 * @param rawSource  A P4 program or program fragment. Callers will normally
 *                   find it convenient to specify this as a raw string.
 * @return the transformed P4 program.
 */
std::string makeP4Source(const char* file, unsigned line,
                         P4Headers headers, const char* rawSource);

/// An overload of makeP4Source which doesn't prepend any headers; equivalent to
/// `makeP4Source(file, line, P4Headers::NONE, rawSource);`.
std::string makeP4Source(const char* file, unsigned line, const char* rawSource);

}  // namespace detail

// A macro which should be used by unit tests to define P4 source code. It adds
// additional information to the source code to aid in debugging; see
// makeP4Source for more information and parameter details.
#define P4_SOURCE(...) detail::makeP4Source(__FILE__, __LINE__, __VA_ARGS__)

class P4CTestEnvironment {
    // XXX(seth): Ideally this would be a ::testing::Environment subclass, but
    // if you register a global test environment with GTest it tries to tear it
    // down in an atexit() handler, and in some configurations libgc does the
    // same, resulting in a double delete that's not easy to resolve cleanly.
 public:
    /// @return the global instance of P4CTestEnvironment.
    static P4CTestEnvironment* get();

    /// @return a string containing the "core.p4" P4 standard header.
    const std::string& coreP4() const { return _coreP4; }

    /// @return a string containing the "v1model.p4" P4 standard header.
    const std::string& v1Model() const { return _v1Model; }

    /// @return a string containing the "psa.p4" P4 standard header.
    const std::string& psaP4() const { return _psaP4; }

 private:
    P4CTestEnvironment();

    std::string _coreP4;
    std::string _v1Model;
    std::string _psaP4;
};

using GTestContext = P4CContextWithOptions<CompilerOptions>;

namespace Test {

/// A test fixture base class that automatically creates a new compilation
/// context for the test to run in.
class P4CTest : public ::testing::Test {
 public:
    P4CTest() : autoGTestContext(new GTestContext(GTestContext::get())) { }

 private:
    AutoCompileContext autoGTestContext;
};

struct FrontendTestCase {
    static const CompilerOptions::FrontendVersion defaultVersion =
        CompilerOptions::FrontendVersion::P4_16;

    /// Create a test case that only requires the frontend to run.
    static boost::optional<FrontendTestCase>
    create(const std::string& source,
           CompilerOptions::FrontendVersion langVersion = defaultVersion,
           P4::ParseAnnotations parseAnnotations = P4::ParseAnnotations());

    static boost::optional<FrontendTestCase>
    create(const std::string& source, P4::ParseAnnotations parseAnnotations) {
        return create(source, defaultVersion, parseAnnotations);
    }

    /// The output of the frontend.
    const IR::P4Program* program;
};

}  // namespace Test

#endif /* TEST_GTEST_HELPERS_H_ */
