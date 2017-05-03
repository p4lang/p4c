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

#ifndef _FRONTENDS_COMMON_PARSEINPUT_H_
#define _FRONTENDS_COMMON_PARSEINPUT_H_

#include "options.h"

namespace IR {
class P4Program;
}  // namespace IR

namespace P4 {

/**
 * Parse P4 source from a file. The filename and language version are specified
 * by @options. If the language version is not P4-16, then the program is
 * converted to P4-16 before being returned.
 *
 * The program state is cleared before parsing begins. This is normally what you
 * want. If you need more control (for example, if you need to compose a larger
 * program out of fragments that come from different input sources) you can use
 * a ParserDriver directly.
 *
 * @return a P4-16 IR tree representing the contents of the given file, or null
 * on failure. If failure occurs, an error will also be reported.
 */
const IR::P4Program* parseP4File(CompilerOptions& options);

/**
 * Parse P4 source from the string @input, interpreting it as having language
 * version @version. The source is not preprocessed before being parsed. If the
 * language version is not P4-16, then the program is converted to P4-16 before
 * being returned.
 *
 * The program state is cleared before parsing begins. This is normally what you
 * want. If you need more control (for example, if you need to compose a larger
 * program out of fragments that come from different input sources) you can use
 * a ParserDriver directly.
 *
 * @return a P4-16 IR tree representing the contents of the given string, or
 * null on failure. If failure occurs, an error will also be reported.
 */
const IR::P4Program* parseP4String(const std::string& input,
                                   CompilerOptions::FrontendVersion version);

/**
 * Clear global program state so that a new program can be parsed.
 *
 * Since parseP4File() and parseP4String() handle this for you, you probably
 * won't need to use this unless you're using a ParserDriver directly.
 */
void clearProgramState();

}  // namespace P4

#endif /* _FRONTENDS_COMMON_PARSEINPUT_H_ */
