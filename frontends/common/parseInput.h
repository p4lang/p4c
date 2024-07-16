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

#ifndef FRONTENDS_COMMON_PARSEINPUT_H_
#define FRONTENDS_COMMON_PARSEINPUT_H_

#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/fromv1.0/converters.h"
#include "frontends/parsers/parserDriver.h"
#include "lib/error.h"

namespace IR {
class P4Program;
}  // namespace IR

namespace P4 {

template <typename Input, typename C = P4V1::Converter>
static const IR::P4Program *parseV1Program(Input stream, std::string_view sourceFile,
                                           unsigned sourceLine,
                                           std::optional<DebugHook> debugHook = std::nullopt) {
    // We load the model before parsing the input file, so that the SourceInfo
    // in the model comes first.
    C converter;
    if (debugHook) converter.addDebugHook(*debugHook, true);
    converter.loadModel();

    // Parse.
    const IR::Node *v1 = V1::V1ParserDriver::parse(stream, sourceFile, sourceLine);
    if (::errorCount() > 0 || v1 == nullptr) return nullptr;

    // Convert to P4-16.
    if (Log::verbose()) std::cerr << "Converting to P4-16" << std::endl;
    v1 = v1->apply(converter);
    if (::errorCount() > 0 || v1 == nullptr) return nullptr;
    BUG_CHECK(v1->is<IR::P4Program>(), "Conversion returned %1%", v1);
    return v1->to<IR::P4Program>();
}

/**
 * Parse P4 source from a file. The filename and language version are specified
 * by @options. If the language version is not P4-16, then the program is
 * converted to P4-16 before being returned.
 *
 * @return a P4-16 IR tree representing the contents of the given file, or null
 * on failure. If failure occurs, an error will also be reported.
 */
template <typename C = P4V1::Converter>
const IR::P4Program *parseP4File(const ParserOptions &options) {
    BUG_CHECK(&options == &P4CContext::get().options(),
              "Parsing using options that don't match the current "
              "compiler context");

    const IR::P4Program *result = nullptr;
    if (options.doNotPreprocess) {
        auto *file = fopen(options.file.c_str(), "r");
        if (file == nullptr) {
            ::error(ErrorType::ERR_NOT_FOUND, "%1%: No such file or directory.", options.file);
            return nullptr;
        }
        result = options.isv1() ? parseV1Program<FILE *, C>(file, options.file.string(), 1,
                                                            options.getDebugHook())
                                : P4ParserDriver::parse(file, options.file.string());
        fclose(file);
    } else {
        auto preprocessorResult = options.preprocess();
        if (::errorCount() > 0 || !preprocessorResult.has_value()) {
            return nullptr;
        }
        // Need to assign file here because the parser requires an lvalue.
        result =
            options.isv1()
                ? parseV1Program<FILE *, C>(preprocessorResult.value().get(), options.file.string(),
                                            1, options.getDebugHook())
                : P4ParserDriver::parse(preprocessorResult.value().get(), options.file.string());
    }

    if (::errorCount() > 0) {
        ::error(ErrorType::ERR_OVERLIMIT, "%1% errors encountered, aborting compilation",
                ::errorCount());
        return nullptr;
    }
    BUG_CHECK(result != nullptr, "Parsing failed, but we didn't report an error");
    return result;
}

/**
 * Parse P4 source from the string @input, interpreting it as having language
 * version @version. The source is not preprocessed before being parsed; the
 * initial source location is derived from @sourceFile and @sourceLine. If the
 * language version is not P4-16, then the program is converted to P4-16 before
 * being returned.
 *
 * @return a P4-16 IR tree representing the contents of the given string, or
 * null on failure. If failure occurs, an error will also be reported.
 */
const IR::P4Program *parseP4String(const char *sourceFile, unsigned sourceLine,
                                   const std::string &input,
                                   CompilerOptions::FrontendVersion version);
const IR::P4Program *parseP4String(const std::string &input,
                                   CompilerOptions::FrontendVersion version);

}  // namespace P4

#endif /* FRONTENDS_COMMON_PARSEINPUT_H_ */
