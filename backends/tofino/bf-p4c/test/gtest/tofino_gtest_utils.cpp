/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/common/extract_maupipe.h"
#include "bf-p4c/common/front_end_policy.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/parse_annotations.h"
#include "bf-p4c/midend.h"
#include "bf-p4c/phv/create_thread_local_instances.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

const char *tna_header() {
    static std::string tna =
        P4CTestEnvironment::readHeader("p4include/tna.p4", true, "__TARGET_TOFINO__", 1);
    return tna.c_str();
}

const char *t2na_header() {
    static std::string t2na =
        P4CTestEnvironment::readHeader("p4include/t2na.p4", true, "__TARGET_TOFINO__", 2);
    return t2na.c_str();
}

/* static */ std::optional<MidendTestCase> MidendTestCase::create(const std::string &source) {
    AutoCompileContext autoBFNContext(new BFNContext(BFNContext::get()));
    auto &options = BackendOptions();

    auto parseAnnotations = BFN::ParseAnnotations();
    auto policy = BFN::FrontEndPolicy(&parseAnnotations, true);
    auto frontendTestCase = FrontendTestCase::create(source, &policy);
    if (!frontendTestCase) return std::nullopt;
    frontendTestCase->program->apply(BFN::FindArchitecture());

    BFN::MidEnd midend(options);
    auto *midendProgram = frontendTestCase->program->apply(midend);
    if (midendProgram == nullptr) {
        std::cerr << "Midend failed" << std::endl;
        return std::nullopt;
    }
    if (::diagnosticCount() > 0) {
        std::cerr << "Encountered " << ::diagnosticCount() << " errors while executing midend"
                  << std::endl;
        return std::nullopt;
    }

    return MidendTestCase{midendProgram, frontendTestCase->program};
}

/* static */ std::optional<TofinoPipeTestCase> TofinoPipeTestCase::create(
    const std::string &source) {
    AutoCompileContext autoBFNContext(new BFNContext(BFNContext::get()));
    auto &options = BackendOptions();

    auto parseAnnotations = BFN::ParseAnnotations();
    auto policy = BFN::FrontEndPolicy(&parseAnnotations, true);
    auto frontendTestCase = FrontendTestCase::create(source, options.langVersion, &policy);
    if (!frontendTestCase) return std::nullopt;
    frontendTestCase->program->apply(BFN::FindArchitecture());

    BFN::MidEnd midend(options);
    auto *midendProgram = frontendTestCase->program->apply(midend);
    if (midendProgram == nullptr) {
        std::cerr << "Midend failed" << std::endl;
        return std::nullopt;
    }
    if (::errorCount() > 0) {
        std::cerr << "Encountered " << ::errorCount() << " errors while executing midend"
                  << std::endl;
        return std::nullopt;
    }
    // no-op
    ordered_map<cstring, const IR::Type_StructLike *> empty;
    SubstitutePackedHeaders postmid(options, empty, *midend.sourceInfoLogging);
    midendProgram->apply(postmid);
    if (postmid.pipe.size() == 0) {
        std::cerr << "backend converter failed" << std::endl;
        return std::nullopt;
    }
    auto *pipe = postmid.pipe[0];
    if (pipe == nullptr) {
        std::cerr << "extract_maupipe failed" << std::endl;
        return std::nullopt;
    }
    if (::errorCount() > 0) {
        std::cerr << "Encountered " << ::errorCount() << " errors while executing extract_maupipe"
                  << std::endl;
        return std::nullopt;
    }

    return TofinoPipeTestCase{pipe, midendProgram};
}

/* static */ std::optional<TofinoPipeTestCase> TofinoPipeTestCase::createWithThreadLocalInstances(
    const std::string &source) {
    auto pipeTestCase = TofinoPipeTestCase::create(source);
    if (!pipeTestCase) return std::nullopt;

    pipeTestCase->pipe = pipeTestCase->pipe->apply(CreateThreadLocalInstances());
    if (pipeTestCase->pipe == nullptr) {
        std::cerr << "Inserting thread local instances failed" << std::endl;
        return std::nullopt;
    }
    if (::diagnosticCount() > 0) {
        std::cerr << "Encountered " << ::diagnosticCount()
                  << " errors while executing CreateThreadLocalInstances" << std::endl;
        return std::nullopt;
    }

    return pipeTestCase;
}

}  // namespace P4::Test
