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

#include <string>

namespace detail {

/**
 * Creates a "nice" version of the P4 program in @rawSource which contains
 * additional information for the preprocessor to aid in debugging. @file and
 * @line should be the __FILE__ and __LINE__, respectively, at which @rawSource
 * is defined.
 */
std::string makeP4Source(const char* file, unsigned line, const char* rawSource);

}  // namespace detail

// A macro which should be used by unit tests to define P4 source code. It adds
// additional information to the source code to aid in debugging; see
// makeP4Source for more information.
#define P4_SOURCE(SRC) detail::makeP4Source(__FILE__, __LINE__, SRC)

/* preprocessing by prepending the content of core.p4 to test program */
std::string with_core_p4(const std::string& pgm);

#endif /* TEST_GTEST_HELPERS_H_ */
