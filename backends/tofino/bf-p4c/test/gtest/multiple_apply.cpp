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

#include <optional>
#include <type_traits>
#include <boost/algorithm/string/replace.hpp>
#include "gtest/gtest.h"

#include "ir/ir.h"
#include "lib/error.h"
#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "test/gtest/helpers.h"


namespace P4::Test {

class MultipleApplyTest : public TofinoBackendTest {};

std::optional<TofinoPipeTestCase> createMultipleApplyTest(const std::string& ingressApply) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header data_t {
            bit<16> h1;
            bit<16> h2;
            bit<16> h3;
            bit<8>  b1;
            bit<8>  b2;
            bit<8>  b3;
            bit<8>  b4;
            bit<8>  b5;
        }

        struct metadata {
bit<2> a;
bit<1> b;
        }

        struct headers { data_t data; }


        parser parse(packet_in packet, out headers hdr, inout metadata meta,
                 inout standard_metadata_t sm) {
            state start {
                packet.extract(hdr.data);
                transition accept;
            }
        }

        control verifyChecksum(inout headers hdr, inout metadata meta) { apply { } }
        control ingress(inout headers hdr, inout metadata meta,
                        inout standard_metadata_t sm) {

            action noop() {}

            action t1_act(bit<8> b3) {
                hdr.data.b3 = b3;
            }

            action t2_act(bit<8> b4) {
                hdr.data.b4 = b4;
            }

            action t3_act(bit<8> b5) {
                hdr.data.b5 = b5;
            }

            table t1 {
                actions = { t1_act;
                            noop; }
            key = { hdr.data.h1 : exact; }
                default_action = noop;
                size = 512;
            }

            table t2 {
                actions = { t2_act;
                            noop; }
                key = { hdr.data.h2: exact; }
                default_action = noop;
                size = 512;
            }

            table t3 {
                actions = { t3_act;
                            noop; }
                key = { hdr.data.h3: exact; }
                default_action = noop;
                size = 512;
            }


            apply {
%INGRESS_APPLY%
            }

        }

        control egress(inout headers hdr, inout metadata meta,
                       inout standard_metadata_t sm) {
            apply { } }

        control computeChecksum(inout headers hdr, inout metadata meta) {
            apply { } }

        control deparse(packet_out packet, in headers hdr) {
            apply {
                packet.emit(hdr.data);
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;

    )");

    boost::replace_first(source, "%INGRESS_APPLY%", ingressApply);

    auto& options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::create(source);
}

/** Because the control flow of t2 and t3 are different, these sequences are not replaceable
 *  with each other.
 */
TEST_F(MultipleApplyTest, NonMatchingSequences) {
    auto test = createMultipleApplyTest(P4_SOURCE(P4Headers::NONE, R"(
        if (hdr.data.b1 == 0) {
            if (!t1.apply().hit) {
                t2.apply();
                t3.apply();
            }
        } else {
            t3.apply();
            t2.apply();
        }
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_TRUE(ma.topological_error("ingress.t2"_cs));
    EXPECT_TRUE(ma.topological_error("ingress.t3"_cs));
    EXPECT_FALSE(ma.topological_error("ingress.t1"_cs));
}

/** This is just an example that should completely work */
TEST_F(MultipleApplyTest, ChainedApplications) {
    auto test = createMultipleApplyTest(P4_SOURCE(P4Headers::NONE, R"(
        if (hdr.data.b1 == 0) {
            if (!t1.apply().hit) {
                if (hdr.data.b2 == 0) {
                    if (!t2.apply().hit) {
                        t3.apply();
                    }
                } else {
                    t3.apply();
                }
            }
        } else {
            if (hdr.data.b2 == 0) {
                if (!t2.apply().hit) {
                    t3.apply();
                }
            } else {
                t3.apply();
            }
        }
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_FALSE(ma.mutex_error("ingress.t1"_cs));
    EXPECT_FALSE(ma.mutex_error("ingress.t2"_cs));
    EXPECT_FALSE(ma.mutex_error("ingress.t3"_cs));
    EXPECT_FALSE(ma.topological_error("ingress.t1"_cs));
    EXPECT_FALSE(ma.topological_error("ingress.t2"_cs));
    EXPECT_FALSE(ma.topological_error("ingress.t3"_cs));
    EXPECT_FALSE(ma.default_next_error("ingress.t1"_cs));
    EXPECT_FALSE(ma.default_next_error("ingress.t2"_cs));
    EXPECT_FALSE(ma.default_next_error("ingress.t3"_cs));
}

/** Currently direct action calls are created as separate tables, and some equivalence check
 *  would be needed on these tables to wrap them together.  Unnecessary in the short-term
 *  for p4_14 anyways, as this functionality is only in p4_16
 */
TEST_F(MultipleApplyTest, DirectAction) {
    auto test = createMultipleApplyTest(P4_SOURCE(P4Headers::NONE, R"(
        if (hdr.data.b1 == 0) {
            if (!t1.apply().hit) {
                t2.apply();
                t1_act(0x34);
            }
        } else {
            t2.apply();
            t1_act(0x34);
        }
        t3.apply();
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_FALSE(ma.topological_error("ingress.t2"_cs));
    EXPECT_TRUE(ma.default_next_error("ingress.t2"_cs));
}

/** All applies of a table must be mutually exclusive */
TEST_F(MultipleApplyTest, NonMutuallyExclusive) {
    auto test = createMultipleApplyTest(P4_SOURCE(P4Headers::NONE, R"(
        if (hdr.data.b1 == 0) {
            t2.apply();
        }

        if (hdr.data.b1 == 0) {
            t2.apply();
        }

        t1.apply();
        t3.apply();
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_TRUE(ma.mutex_error("ingress.t2"_cs));
}

/** Even though the conditionals of t2 are logically exclusive, the mutually exclusive algorithm
 *  is not smart enough yet to figure this out
 */
TEST_F(MultipleApplyTest, LogicallyMutuallyExclusive) {
    auto test = createMultipleApplyTest(P4_SOURCE(P4Headers::NONE, R"(
        if (hdr.data.b1 == 0) {
            t2.apply();
        }

        if (hdr.data.b1 == 1) {
            t2.apply();
        }

        t1.apply();
        t3.apply();
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_TRUE(ma.mutex_error("ingress.t2"_cs));
}

/** The tail of the TableSeqs match up, so we can split off the common tail giviing a
 *  common invokation of the table
 */
TEST_F(MultipleApplyTest, CommonTail) {
    auto test = createMultipleApplyTest(P4_SOURCE(P4Headers::NONE, R"(
        if (hdr.data.b1 == 0) {
            t1.apply();
            t2.apply();
        } else {
            t2.apply();
        }
        t3.apply();
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_FALSE(ma.topological_error("ingress.t2"_cs));
}

/** The tail of the TableSeqs match up, so we can split off the common tail giviing a
 *  common invokation of the table
 */
TEST_F(MultipleApplyTest, CommonTail2) {
    auto test = createMultipleApplyTest(P4_SOURCE(P4Headers::NONE, R"(
        if (hdr.data.b1 == 0) {
            t1.apply();
            t3.apply();
        } else {
            t2.apply();
            t3.apply();
        }
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_FALSE(ma.topological_error("ingress.t3"_cs));
}

std::optional<TofinoPipeTestCase> createMultipleApplyFullIngTest(const std::string& ingress) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header data_t {
            bit<16> h1;
            bit<16> h2;
            bit<16> h3;
            bit<8>  b1;
            bit<8>  b2;
            bit<8>  b3;
            bit<8>  b4;
            bit<8>  b5;
        }

        struct metadata {
bit<2> a;
bit<1> b;
        }

        struct headers { data_t data; }


        parser parse(packet_in packet, out headers hdr, inout metadata meta,
                 inout standard_metadata_t sm) {
            state start {
                packet.extract(hdr.data);
                transition accept;
            }
        }

        control verifyChecksum(inout headers hdr, inout metadata meta) { apply { } }
        control ingress(inout headers hdr, inout metadata meta,
                        inout standard_metadata_t sm) {
            %INGRESS%
        }

        control egress(inout headers hdr, inout metadata meta,
                       inout standard_metadata_t sm) {
            apply { } }

        control computeChecksum(inout headers hdr, inout metadata meta) {
            apply { } }

        control deparse(packet_out packet, in headers hdr) {
            apply {
                packet.emit(hdr.data);
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;

    )");

    boost::replace_first(source, "%INGRESS%", ingress);

    auto& options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;
    return TofinoPipeTestCase::create(source);
}


TEST_F(MultipleApplyTest, SWI_2941_example1) {
    auto test = createMultipleApplyFullIngTest(P4_SOURCE(P4Headers::NONE, R"(
        action noop() {}
        action a1(bit<8> b3) {
            hdr.data.b3 = b3;
        }

        table A {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table B {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table C {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table D {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table E {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table F {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }
        
        table G {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        apply {
            if (A.apply().hit) {
                if (hdr.data.h2 == 0) {
                    B.apply();
                    E.apply();
                } else if (hdr.data.h2 == 1) {
                    C.apply();
                    F.apply();
                }
            } else {
                 D.apply();
                 E.apply();
                 F.apply();
            }
            G.apply();
        }
        
    )"_cs));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_TRUE(ma.default_next_error("ingress.E"_cs));
}

TEST_F(MultipleApplyTest, SWI_2941_example2) {
    auto test = createMultipleApplyFullIngTest(P4_SOURCE(P4Headers::NONE, R"(
        action noop() {}
        action a1(bit<8> b3) {
            hdr.data.b3 = b3;
        }

        table A {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table B {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table C {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table D {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table E {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table F {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }
        
        table G {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        apply {
            if (A.apply().hit) {
                if (hdr.data.h2 == 0) {
                    B.apply();
                    E.apply();
                    F.apply();
                } else if (hdr.data.h2 == 1) {
                    C.apply();
                    F.apply();
                }
            } else {
                 D.apply();
                 E.apply();
                 F.apply();
            }
            G.apply();
        }
        
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_FALSE(ma.default_next_error("ingress.E"_cs));
}

TEST_F(MultipleApplyTest, SWI_2941_example3) {
    auto test = createMultipleApplyFullIngTest(P4_SOURCE(P4Headers::NONE, R"(
        action noop() {}
        action a1(bit<8> b3) {
            hdr.data.b3 = b3;
        }

        table A {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table B {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table C {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table D {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table E {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table F {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }
        
        table G {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        apply {
            if (A.apply().hit) {
                if (hdr.data.h2 == 0) {
                    B.apply();
                    E.apply();
                    if (hdr.data.h1 == 0)
                        F.apply();
                } else if (hdr.data.h2 == 1) {
                    C.apply();
                    if (hdr.data.h1 == 1)
                        F.apply();
                }
            } else {
                 D.apply();
                 E.apply();
                 if (hdr.data.h1 == 0)
                     F.apply();
            }
            G.apply();
        }
        
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_FALSE(ma.default_next_error("ingress.E"_cs));
}

TEST_F(MultipleApplyTest, SWI_2941_example4) {
    auto test = createMultipleApplyFullIngTest(P4_SOURCE(P4Headers::NONE, R"(
        action noop() {}
        action a1(bit<8> b3) {
            hdr.data.b3 = b3;
        }

        table A {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table B {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table C {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table D {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table E {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table F {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }
        
        table G {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        apply {
            if (A.apply().hit) {
                if (hdr.data.h2 == 0) {
                    B.apply();
                    if (hdr.data.h1 == 0)
                        E.apply();
                } else if (hdr.data.h2 == 1) {
                    C.apply();
                    if (hdr.data.h1 == 1)
                        F.apply();
                }
            } else {
                 D.apply();
                 if (hdr.data.h1 == 1)
                     E.apply();
                 if (hdr.data.h1 == 0)
                     F.apply();
            }
            G.apply();
        }
        
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_TRUE(ma.default_next_error("ingress.E"_cs));
}

/**
 * Corresponds to the comments written in MultipleApply above check_all_gws, to validate
 * that pass.
 */
TEST_F(MultipleApplyTest, SWI_2941_example5) {
    auto test = createMultipleApplyFullIngTest(P4_SOURCE(P4Headers::NONE, R"(
        action noop() {}
        action a1(bit<8> b3) {
            hdr.data.b3 = b3;
        }

        table A {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table B {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        table C {
            actions = { noop; a1; }
            key = { hdr.data.h1 : exact; }
            default_action = noop;
            size = 512;
        }

        apply {
            if (A.apply().hit) {
                if (hdr.data.h2 == 0) {
                    B.apply();
                    if (hdr.data.h1 == 0)
                        C.apply();
                } else if (hdr.data.h1 == 0) {
                    C.apply();
                }
            } else {
                 if (hdr.data.h2 == 1)
                     B.apply();
                 if (hdr.data.h1 == 0)
                     C.apply();
            }
        }
    )"));

    MultipleApply ma(BackendOptions());
    test->pipe->apply(ma);
    EXPECT_FALSE(ma.default_next_error("ingress.A"_cs));
    EXPECT_FALSE(ma.default_next_error("ingress.B"_cs));
    EXPECT_FALSE(ma.default_next_error("ingress.C"_cs));
}

}  // namespace P4::Test
