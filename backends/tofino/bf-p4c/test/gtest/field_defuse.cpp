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

#include "bf-p4c/common/field_defuse.h"

#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class FieldDefUseTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createFieldDefUseTestCase(const std::string &ingress) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H1
        {
            bit<9> f1;
            bit<7> f2;
        }
        struct Headers { H1 h1; }
        struct Metadata { bit<8> f1; }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                     inout standard_metadata_t sm) {
            state start {
            packet.extract(headers.h1);
            transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control ingress(inout Headers headers, inout Metadata meta,
                        inout standard_metadata_t sm) { apply { 
                            %INGRESS%
                             } }
        control egress(inout Headers headers, inout Metadata meta,
                        inout standard_metadata_t sm) { apply { } }

        control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply { }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%INGRESS%", ingress);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runMockPasses(const IR::BFN::Pipe *pipe, PhvInfo &phv, FieldDefUse &defuse) {
    PassManager quick_backend = {new CollectHeaderStackInfo, new CollectPhvInfo(phv), &defuse};
    return pipe->apply(quick_backend);
}

}  // namespace

TEST_F(FieldDefUseTest, SimpleTest) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<9> foo;
                         if(sm.ingress_port == 1) {
                             foo = 1;
                         } else {
                             foo = 2;
                         }
                         sm.egress_spec = foo + 1;
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 2U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, SimpleSliceTest) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         foo[15:9] = 0;
                         foo[8:0] = sm.ingress_port;
                         if (foo != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 2U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexSliceTest1) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo = 2;
                         } else {
                             foo [7:0] = 3;
                         }
                         if (foo != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 3U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexSliceTest2) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:8] = 2;
                         } else {
                             foo [7:0] = 3;
                         }
                         if (foo != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 3U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexSliceTest3) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:8] = 2;
                         }
                         foo[7:0] = 4;
                         if (foo != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 3U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexSliceTest4) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:8] = 2;
                         } else {
                             foo = 3;
                         }
                         foo[7:1] = 4;
                         if (foo != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 4U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexSliceTest5) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:10] = 2;
                         } else {
                             foo[15:10] = 3;
                         }
                         foo[13:11] = 4;
                         foo[12:9] = 5;
                         foo[7:2] = 6;
                         foo[3:3] = 0;
                         foo[5:5] = 1;
                         if (foo != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 8U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexSliceTest6) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:10] = 2;
                         } else {
                             foo[15:10] = 3;
                         }
                         foo[13:11] = 4;
                         foo[12:9] = 5;
                         foo[7:2] = 6;
                         foo[3:3] = 0;
                         foo[13:13] = 1;
                         if (foo != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 7U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexUseSlice1) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:10] = 2;
                         } else {
                             foo[15:10] = 3;
                         }
                         if (foo[15:10] != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 2U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexUseSlice2) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:8] = 2;
                         } else {
                             foo [7:0] = 3;
                         }
                         if (foo[7:0] != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 2U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}
TEST_F(FieldDefUseTest, ComplexUseSlice3) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:8] = 2;
                         } else {
                             foo[7:0] = 3;
                         }
                         if (foo[10:5] != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 3U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexUseSlice4) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:10] = 2;
                         } else {
                             foo[15:10] = 3;
                         }
                         foo[13:11] = 4;
                         foo[12:9] = 5;
                         foo[7:2] = 6;
                         foo[3:3] = 0;
                         foo[5:5] = 1;
                         if (foo[5:0] != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 4U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

TEST_F(FieldDefUseTest, ComplexUseSlice5) {
    auto test = createFieldDefUseTestCase(P4_SOURCE(P4Headers::NONE,
                                                    R"(
                         bit<16> foo = 1;
                         if (sm.ingress_port == 1) {
                             foo[15:10] = 2;
                         } else {
                             foo[15:10] = 3;
                         }
                         foo[13:11] = 4;
                         foo[12:9] = 5;
                         foo[7:2] = 6;
                         foo[3:3] = 0;
                         foo[5:5] = 1;
                         if (foo[10:3] != 1) {
                             sm.egress_port = 1;
                         } else {
                             sm.egress_port = 2;
                         }
                         )"));
    ASSERT_TRUE(test);
    PhvInfo phv;
    FieldDefUse defuse(phv);
    runMockPasses(test->pipe, phv, defuse);
    auto field = phv.field("ingress::foo_0"_cs);
    ASSERT_TRUE(field);
    auto uses = defuse.getAllUses(field->id);
    ASSERT_EQ(uses.size(), 1U);
    auto use = uses.front();
    auto defs = defuse.getDefs(use);
    ASSERT_EQ(defs.size(), 5U);
    for (auto def : defs) {
        auto tbl = def.first->to<IR::MAU::Table>();
        ASSERT_TRUE(tbl);
        ASSERT_TRUE(tbl->name.startsWith("tbl_field_defuse"));
    }
}

}  // namespace P4::Test
