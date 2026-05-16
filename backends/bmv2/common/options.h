/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_COMMON_OPTIONS_H_
#define BACKENDS_BMV2_COMMON_OPTIONS_H_

#include <getopt.h>

#include "frontends/common/options.h"
#include "lib/options.h"

namespace P4::BMV2 {

class BMV2Options : public CompilerOptions {
 public:
    /// Generate externs.
    bool emitExterns = false;
    /// File to output to.
    std::filesystem::path outputFile;
    /// Read from json.
    bool loadIRFromJson = false;

    BMV2Options() {
        registerOption(
            "--emit-externs", nullptr,
            [this](const char *) {
                emitExterns = true;
                return true;
            },
            "[BMv2 back-end] Force externs be emitted by the backend.\n"
            "The generated code follows the BMv2 JSON specification.");
        registerOption(
            "-o", "outfile",
            [this](const char *arg) {
                outputFile = arg;
                return true;
            },
            "Write output to outfile");
        registerOption(
            "--fromJSON", "file",
            [this](const char *arg) {
                loadIRFromJson = true;
                file = arg;
                return true;
            },
            "Use IR representation from JsonFile dumped previously,"
            "the compilation starts with reduced midEnd.");
    }
};

using BMV2Context = P4CContextWithOptions<BMV2Options>;

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_COMMON_OPTIONS_H_ */
