/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_COMMON_APPLYOPTIONSPRAGMAS_H_
#define FRONTENDS_COMMON_APPLYOPTIONSPRAGMAS_H_

#include <optional>
#include <vector>

#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4 {

/// An interface for compiler option pragma parsers; used to customize the
/// behavior of ApplyOptionsPragmas.
class IOptionPragmaParser {
 public:
    using CommandLineOptions = std::vector<const char *>;

    virtual std::optional<CommandLineOptions> tryToParse(const IR::Annotation *annotation) = 0;
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
    explicit ApplyOptionsPragmas(IOptionPragmaParser &parser);

    bool preorder(const IR::Annotation *annotation) override;
    void end_apply() override;

 private:
    IOptionPragmaParser &parser;
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
    bool supportCommandLinePragma;

 public:
    std::optional<CommandLineOptions> tryToParse(const IR::Annotation *annotation) override;

    explicit P4COptionPragmaParser(bool supportCommandLinePragma)
        : supportCommandLinePragma(supportCommandLinePragma) {}

 private:
    std::optional<CommandLineOptions> parseDiagnostic(const IR::Annotation *annotation);
};

}  // namespace P4

#endif /* FRONTENDS_COMMON_APPLYOPTIONSPRAGMAS_H_ */
