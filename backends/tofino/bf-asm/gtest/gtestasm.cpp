/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <gtest/gtest.h>

#include "lib/compile_context.h"
#include "lib/log.h"
#include "lib/options.h"

using namespace P4;

template <typename OptionsType>
class CompileContext : public virtual BaseCompileContext {
 public:
    /// @return the current compilation context, which must be of type
    /// CompileContext<OptionsType>.
    static CompileContext &get() { return CompileContextStack::top<CompileContext>(); }

    CompileContext() {}

    template <typename OptionsDerivedType>
    CompileContext(CompileContext<OptionsDerivedType> &context)
        : optionsInstance(context.options()) {}

    /// @return the compiler options for this compilation context.
    OptionsType &options() { return optionsInstance; }

 private:
    /// The compiler options for this compilation context.
    OptionsType optionsInstance;
};

class GTestOptions : public Util::Options {
    static const char *defaultMessage;

 public:
    GTestOptions() : Util::Options(defaultMessage) {
        registerOption(
            "-T", "loglevel",
            [](const char *arg) {
                Log::addDebugSpec(arg);
                return true;
            },
            "[Compiler debugging] Adjust logging level per file (see below)");
    }
    std::vector<const char *> *process(int argc, char *const argv[]) {
        auto remainingOptions = Util::Options::process(argc, argv);
        return remainingOptions;
    }
    const char *getIncludePath() const override { return ""; }
};

const char *GTestOptions::defaultMessage = "bf-asm gtest";

using GTestContext = CompileContext<GTestOptions>;

GTEST_API_ int main(int argc, char **argv) {
    printf("running gtestasm\n");

    // process gtest flags
    ::testing::InitGoogleTest(&argc, argv);

    // process debug flags
    AutoCompileContext autoGTestContext(new GTestContext);
    GTestContext::get().options().process(argc, argv);

    return RUN_ALL_TESTS();
}
