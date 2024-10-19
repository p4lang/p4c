/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
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
