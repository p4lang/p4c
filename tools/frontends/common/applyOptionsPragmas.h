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

#ifndef FRONTENDS_COMMON_APPLYOPTIONSPRAGMAS_H_
#define FRONTENDS_COMMON_APPLYOPTIONSPRAGMAS_H_

#include <boost/optional.hpp>
#include <vector>
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4 {

/// An interface for compiler option pragma parsers; used to customize the
/// behavior of ApplyOptionsPragmas.
class IOptionPragmaParser {
 public:
    using CommandLineOptions = std::vector<const char*>;

    virtual boost::optional<CommandLineOptions>
    tryToParse(const IR::Annotation* annotation) = 0;
};

/**
 * Find P4-14 pragmas or P4-16 annotations which specify compiler or diagnostic
 * options and generate a sequence of command-line-like arguments which can be
 * processed by CompilerOptions or its subclasses.
 *
 * The analysis of the pragmas themselves is delegated to an IOptionPragmaParser
 * object which can be customized by backends as needed.
 *
 * Although P4-16 annotations are attached to a specific program construct, this
 * pass doesn't care what they're attached to; only their order in the program
 * matters.  This allows these annotations to be used as if they were top-level,
 * standalone directives, and provides a natural translation path from P4-14,
 * where truly standalone pragmas are often used. Annotations which really are
 * specific to a certain program construct should be handled using a different
 * approach.
 */
class ApplyOptionsPragmas : public Inspector {
 public:
    explicit ApplyOptionsPragmas(IOptionPragmaParser& parser);

    bool preorder(const IR::Annotation* annotation) override;
    void end_apply() override;

 private:
    IOptionPragmaParser& parser;
    IOptionPragmaParser::CommandLineOptions options;
};

/**
 * An IOptionPragmaParser implementation that supports basic pragmas that all
 * backends can support.
 *
 * P4COptionPragmaParser recognizes:
 *  - `pragma diagnostic [diagnostic name] [disable|warn|error]`
 *  - `@diagnostic([diagnostic name], ["disable"|"warn"|"error"])`
 *
 * Backends that want to add support for additional pragmas will usually want to
 * inherit from this class and delegate `tryToParse()` to the superclass if
 * they're unable to parse a pragma.
 */
class P4COptionPragmaParser : public IOptionPragmaParser {
 public:
    boost::optional<CommandLineOptions>
    tryToParse(const IR::Annotation* annotation) override;

 private:
    boost::optional<CommandLineOptions>
    parseDiagnostic(const IR::Annotation* annotation);
};

}  // namespace P4

#endif /* FRONTENDS_COMMON_APPLYOPTIONSPRAGMAS_H_ */
