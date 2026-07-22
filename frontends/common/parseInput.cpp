// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "parseInput.h"

#include <cstdio>
#include <sstream>

#include "frontends/p4-14/fromv1.0/converters.h"
#include "frontends/parsers/parserDriver.h"
#include "lib/error.h"

namespace P4 {

IR::Ptr<IR::P4Program> parseP4String(const char *sourceFile, unsigned sourceLine,
                                     const std::string &input,
                                     CompilerOptions::FrontendVersion version) {
    std::istringstream stream(input);
    auto result =
        (version == CompilerOptions::FrontendVersion::P4_14)
            ? parseV1Program<std::istringstream &, P4V1::Converter>(stream, sourceFile, sourceLine)
            : P4ParserDriver::parse(stream, sourceFile, sourceLine);

    if (::P4::errorCount() > 0) {
        ::P4::error(ErrorType::ERR_OVERLIMIT, "%1% errors encountered, aborting compilation",
                    ::P4::errorCount());
        return nullptr;
    }
    BUG_CHECK(result != nullptr, "Parsing failed, but we didn't report an error");
    return result;
}

IR::Ptr<IR::P4Program> parseP4String(const std::string &input,
                                     CompilerOptions::FrontendVersion version) {
    return parseP4String("(string)", 1, input, version);
}

}  // namespace P4
