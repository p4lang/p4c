#include "backends/p4tools/modules/testgen/targets/bmv2/test/test_backend/ptf.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4::P4Tools::Test {

using namespace P4::literals;

using ::testing::HasSubstr;
using ::testing::Not;

TableConfig PTFTest::getForwardTableConfig() {
    ActionArg port(new IR::Parameter("port", IR::Direction::None, IR::Type_Bits::get(9)),
                   IR::Constant::get(IR::Type_Bits::get(9), big_int("0x2")));
    std::vector<ActionArg> args({port});
    const auto hit = ActionCall(
        "SwitchIngress.hit"_cs,
        new IR::P4Action("dummy", new IR::ParameterList({}, {}), new IR::BlockStatement()), args);

    auto const *exactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::Constant::get(IR::Type_Bits::get(48), big_int("0x222222222222")));
    auto matches = TableMatchMap({{"hdr.ethernet.dst_addr"_cs, exactMatch}});

    return TableConfig(new IR::P4Table("table", new IR::TableProperties()),
                       {TableRule(matches, TestSpec::NO_PRIORITY, hit, 0)});
}

TableConfig PTFTest::getIPRouteTableConfig() {
    const auto srcAddr =
        ActionArg(new IR::Parameter("srcAddr", IR::Direction::None, IR::Type_Bits::get(32)),
                  IR::Constant::get(IR::Type_Bits::get(32), big_int("0xF81096F")));
    const auto dstAddr =
        ActionArg(new IR::Parameter("dstAddr", IR::Direction::None, IR::Type_Bits::get(48)),
                  IR::Constant::get(IR::Type_Bits::get(48), big_int("0x12176CD3")));
    const auto dstPort =
        ActionArg(new IR::Parameter("dst_port", IR::Direction::None, IR::Type_Bits::get(9)),
                  IR::Constant::get(IR::Type_Bits::get(9), big_int("0x2")));
    const auto args = std::vector<ActionArg>({srcAddr, dstAddr, dstPort});
    const auto nat = ActionCall(
        "SwitchIngress.nat"_cs,
        new IR::P4Action("dummy", new IR::ParameterList({}, {}), new IR::BlockStatement()), args);

    const auto *dipExactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::Constant::get(IR::Type_Bits::get(32), big_int("0xA612895D")));

    const auto *vrfExactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::Constant::get(IR::Type_Bits::get(16), big_int("0x0")));

    TableMatchMap matches({{"vrf"_cs, vrfExactMatch}, {"hdr.ipv4.dst_addr"_cs, dipExactMatch}});

    return TableConfig(new IR::P4Table("table", new IR::TableProperties()),
                       {TableRule(matches, TestSpec::NO_PRIORITY, nat, 0)});
}

/// Create a test spec with an Exact match and print an ptf test.
TEST_F(PTFTest, Ptf01) {
    const auto *pld =
        IR::Constant::get(IR::Type_Bits::get(512),
                          big_int("0x22222222222200060708090a080045000056000100004006f94dc0a8"
                                  "0001c0a8000204d200500000000000000000500220000d2c0000000102"
                                  "03040506070809"));
    const auto *pldIgnMask = IR::Constant::get(
        IR::Type_Bits::get(512), big_int("0xf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f01"
                                         "0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f010"
                                         "f0f0f0f0f0f0f0f0f0f0f0f0f0f0"));
    const auto ingressPacket = Packet(1, pld, pldIgnMask);
    const auto egressPacket = Packet(2, pld, pldIgnMask);

    const auto fwdConfig = getForwardTableConfig();

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables"_cs, "SwitchIngress.forward"_cs, &fwdConfig);

    TestBackendConfiguration testBackendConfiguration{"test01"_cs, 1, "test01", 1};
    auto testWriter = PTF(testBackendConfiguration);
    testWriter.writeTestToFile(&testSpec, cstring::empty, 1, 0);
}

/// Create a test spec with two Exact matches and print an ptf test.
TEST_F(PTFTest, Ptf02) {
    /// TODO: If payload starts with leading 0s, they are truncated causing ptf to fail with
    /// malformed packet, need to pad to account for leading zeros.
    const auto *pldIngress = IR::Constant::get(
        IR::Type_Bits::get(512), big_int("0x22210203040500060708090a0800450000560001000040068"
                                         "a88c0a80001a612895d04d20050000000000000000050022000"
                                         "0000000000010203040506070809"));
    const auto *pldEgress = IR::Constant::get(
        IR::Type_Bits::get(512), big_int("0x22210203040500060708090a080045000056000100004006e"
                                         "2c70f81096f12176cd304d20050000000000000000050022000"
                                         "0000000000010203040506070809"));
    const auto *pldIgnMask = IR::Constant::get(
        IR::Type_Bits::get(512), big_int("0xf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f01"
                                         "0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f010"
                                         "f0f0f0f0f0f0f0f0f0f0f0f0f0f0"));
    const auto ingressPacket = Packet(5, pldIngress, pldIgnMask);
    const auto egressPacket = Packet(2, pldEgress, pldIgnMask);

    const auto fwdConfig = getForwardTableConfig();
    const auto ipRouteConfig = getIPRouteTableConfig();

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables"_cs, "SwitchIngress.forward"_cs, &fwdConfig);
    testSpec.addTestObject("tables"_cs, "SwitchIngress.ipRoute"_cs, &ipRouteConfig);

    TestBackendConfiguration testBackendConfiguration{"test02"_cs, 1, "test02", 2};
    auto testWriter = PTF(testBackendConfiguration);
    testWriter.writeTestToFile(&testSpec, cstring::empty, 2, 0);
}

TableConfig PTFTest::gettest1TableConfig() {
    const auto val = ActionArg(new IR::Parameter("val", IR::Direction::None, IR::Type_Bits::get(8)),
                               IR::Constant::get(IR::Type_Bits::get(8), big_int("0x7F")));
    const auto port =
        ActionArg(new IR::Parameter("port", IR::Direction::None, IR::Type_Bits::get(9)),
                  IR::Constant::get(IR::Type_Bits::get(9), big_int("0x2")));
    const auto args = std::vector<ActionArg>({val, port});
    const auto setb1 = ActionCall(
        "setb1"_cs,
        new IR::P4Action("dummy", new IR::ParameterList({}, {}), new IR::BlockStatement()), args);

    const auto *f1TernaryMatch = new Ternary(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("ternary")),
        IR::Constant::get(IR::Type_Bits::get(32), big_int("0xABCD0101")),
        IR::Constant::get(IR::Type_Bits::get(32), big_int("0xFFFF0000")));

    TableMatchMap matches({{"data.f1"_cs, f1TernaryMatch}});

    return TableConfig(new IR::P4Table("table", new IR::TableProperties()),
                       {TableRule(matches, TestSpec::LOW_PRIORITY, setb1, 0)});
}

/// Create a test spec with a Ternary match and print an ptf test.
TEST_F(PTFTest, Ptf03) {
    const auto *pldIngress =
        IR::Constant::get(IR::Type_Bits::get(112), big_int("0x0000010100000202030355667788"));
    const auto *pldIngIgnMask =
        IR::Constant::get(IR::Type_Bits::get(112), big_int("0x0000000000000000000000000000"));
    const auto *pldEgress =
        IR::Constant::get(IR::Type_Bits::get(96), big_int("0x00000101deadd00dbeef7f66"));
    const auto *pldEgIgnMask =
        IR::Constant::get(IR::Type_Bits::get(96), big_int("0x00000000ffffffffffff0000"));

    const auto ingressPacket = Packet(0, pldIngress, pldIngIgnMask);
    const auto egressPacket = Packet(2, pldEgress, pldEgIgnMask);

    const auto test1Config = gettest1TableConfig();

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables"_cs, "test1"_cs, &test1Config);

    TestBackendConfiguration testBackendConfiguration{"test03"_cs, 1, "test03", 3};
    auto testWriter = PTF(testBackendConfiguration);
    try {
        testWriter.writeTestToFile(&testSpec, cstring::empty, 3, 0);
    } catch (const Util::CompilerBug &e) {
        EXPECT_THAT(e.what(), HasSubstr("Unimplemented for Ternary FieldMatch"));
    }
}

TableConfig PTFTest::gettest1TableConfig2() {
    const auto val = ActionArg(new IR::Parameter("val", IR::Direction::None, IR::Type_Bits::get(8)),
                               IR::Constant::get(IR::Type_Bits::get(8), big_int("0x7F")));
    const auto port =
        ActionArg(new IR::Parameter("port", IR::Direction::None, IR::Type_Bits::get(9)),
                  IR::Constant::get(IR::Type_Bits::get(9), big_int("0x2")));
    const auto args = std::vector<ActionArg>({val, port});
    const auto setb1 = ActionCall(
        "setb1"_cs,
        new IR::P4Action("dummy", new IR::ParameterList({}, {}), new IR::BlockStatement()), args);

    const auto *h1ExactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::Constant::get(IR::Type_Bits::get(16), big_int("0x3")));

    const auto *f1TernaryMatch = new Ternary(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("ternary")),
        IR::Constant::get(IR::Type_Bits::get(32), big_int("0xABCD0101")),
        IR::Constant::get(IR::Type_Bits::get(32), big_int("0xFFFF0000")));

    const auto *b1ExactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::Constant::get(IR::Type_Bits::get(8), big_int("0x1")));

    const auto *f2TernaryMatch = new Ternary(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("ternary")),
        IR::Constant::get(IR::Type_Bits::get(32), big_int("0xABCDD00D")),
        IR::Constant::get(IR::Type_Bits::get(32), big_int("0x0000FFFF")));

    TableMatchMap matches1({{"data.f1"_cs, f1TernaryMatch}});

    TableMatchMap matches2({{"data.h1"_cs, h1ExactMatch},
                            {"data.f1"_cs, f1TernaryMatch},
                            {"data.b1"_cs, b1ExactMatch},
                            {"data.f2"_cs, f2TernaryMatch}});

    return TableConfig(new IR::P4Table("table", new IR::TableProperties()),
                       {TableRule(matches1, TestSpec::LOW_PRIORITY, setb1, 0),
                        TableRule(matches2, TestSpec::HIGH_PRIORITY, setb1, 0)});
}

/// Create a test spec with one Exact match and one Ternary match and print an ptf test.
TEST_F(PTFTest, Ptf04) {
    const auto *pldIngress =
        IR::Constant::get(IR::Type_Bits::get(112), big_int("0x0000010100000202030355667788"));
    const auto *pldIngIgnMask =
        IR::Constant::get(IR::Type_Bits::get(112), big_int("0x0000000000000000000000000000"));
    const auto *pldEgress =
        IR::Constant::get(IR::Type_Bits::get(96), big_int("0x00000101deadd00dbeef7f66"));
    const auto *pldEgIgnMask =
        IR::Constant::get(IR::Type_Bits::get(96), big_int("0x00000000ffffffffffff0000"));

    const auto ingressPacket = Packet(0, pldIngress, pldIngIgnMask);
    const auto egressPacket = Packet(2, pldEgress, pldEgIgnMask);

    const auto test1Config = gettest1TableConfig2();

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables"_cs, "test1"_cs, &test1Config);

    TestBackendConfiguration testBackendConfiguration{"test04"_cs, 1, "test04", 4};
    auto testWriter = PTF(testBackendConfiguration);
    try {
        testWriter.writeTestToFile(&testSpec, cstring::empty, 4, 0);
    } catch (const Util::CompilerBug &e) {
        EXPECT_THAT(e.what(), HasSubstr("Unimplemented for Ternary FieldMatch"));
    }
}

TEST_F(PTFTest, PtfActionSelectorProgramming) {
    const auto *payload =
        IR::Constant::get(IR::Type_Bits::get(112), big_int("0x0000010100000202030355667788"));
    const auto *payloadMask =
        IR::Constant::get(IR::Type_Bits::get(112), big_int("0x0000000000000000000000000000"));
    const auto ingressPacket = Packet(0, payload, payloadMask);
    const auto egressPacket = Packet(2, payload, payloadMask);

    auto selectorConfig = getForwardTableConfig();
    const auto *apDecl = new IR::P4Table("selector_profile_decl", new IR::TableProperties());
    const auto actionProfile = P4Testgen::Bmv2::Bmv2V1ModelActionProfile(apDecl);
    const auto *asDecl = new IR::P4Table("selector_decl", new IR::TableProperties());
    const auto actionSelector = P4Testgen::Bmv2::Bmv2V1ModelActionSelector(asDecl, &actionProfile);
    selectorConfig.addTableProperty("action_profile"_cs, &actionProfile);
    selectorConfig.addTableProperty("action_selector"_cs, &actionSelector);

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables"_cs, "SwitchIngress.forward"_cs, &selectorConfig);

    const auto fileBasePath = std::filesystem::path("/tmp/p4c-bmv2-ptf-selector-test");
    TestBackendConfiguration testBackendConfiguration{"selector_programming"_cs, 1, fileBasePath,
                                                      1};
    auto testWriter = PTF(testBackendConfiguration);
    testWriter.writeTestToFile(&testSpec, cstring::empty, 5, 0);

    auto generatedFile = fileBasePath;
    generatedFile.replace_extension(".py");
    std::ifstream stream(generatedFile);
    ASSERT_TRUE(stream.is_open());
    std::stringstream buffer;
    buffer << stream.rdbuf();
    const auto rendered = buffer.str();
    EXPECT_THAT(rendered, HasSubstr("self.send_request_add_member("));
    EXPECT_THAT(rendered, HasSubstr("self.send_request_add_group("));
    EXPECT_THAT(rendered, HasSubstr("self.send_request_add_entry_to_group("));
    EXPECT_THAT(rendered, HasSubstr("'selector_profile_decl'"));
    EXPECT_THAT(rendered, Not(HasSubstr("'selector_decl'")));
    std::filesystem::remove(generatedFile);
}

TEST_F(PTFTest, PtfActionSelectorSharedProfileIdsAreUniqueAcrossTables) {
    const auto *payload =
        IR::Constant::get(IR::Type_Bits::get(112), big_int("0x0000010100000202030355667788"));
    const auto *payloadMask =
        IR::Constant::get(IR::Type_Bits::get(112), big_int("0x0000000000000000000000000000"));
    const auto ingressPacket = Packet(0, payload, payloadMask);
    const auto egressPacket = Packet(2, payload, payloadMask);

    auto selectorConfig1 = getForwardTableConfig();
    auto selectorConfig2 = getForwardTableConfig();
    const auto *apDecl = new IR::P4Table("hashed_selector", new IR::TableProperties());
    const auto actionProfile = P4Testgen::Bmv2::Bmv2V1ModelActionProfile(apDecl);
    const auto *asDecl1 = new IR::P4Table("selector_decl_1", new IR::TableProperties());
    const auto *asDecl2 = new IR::P4Table("selector_decl_2", new IR::TableProperties());
    const auto actionSelector1 =
        P4Testgen::Bmv2::Bmv2V1ModelActionSelector(asDecl1, &actionProfile);
    const auto actionSelector2 =
        P4Testgen::Bmv2::Bmv2V1ModelActionSelector(asDecl2, &actionProfile);
    selectorConfig1.addTableProperty("action_profile"_cs, &actionProfile);
    selectorConfig1.addTableProperty("action_selector"_cs, &actionSelector1);
    selectorConfig2.addTableProperty("action_profile"_cs, &actionProfile);
    selectorConfig2.addTableProperty("action_selector"_cs, &actionSelector2);

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables"_cs, "SwitchIngress.forward_1"_cs, &selectorConfig1);
    testSpec.addTestObject("tables"_cs, "SwitchIngress.forward_2"_cs, &selectorConfig2);

    const auto fileBasePath = std::filesystem::path("/tmp/p4c-bmv2-ptf-selector-shared-profile");
    TestBackendConfiguration testBackendConfiguration{"selector_shared_profile"_cs, 1, fileBasePath,
                                                      1};
    auto testWriter = PTF(testBackendConfiguration);
    testWriter.writeTestToFile(&testSpec, cstring::empty, 7, 0);

    auto generatedFile = fileBasePath;
    generatedFile.replace_extension(".py");
    std::ifstream stream(generatedFile);
    ASSERT_TRUE(stream.is_open());
    std::stringstream buffer;
    buffer << stream.rdbuf();
    const auto rendered = buffer.str();
    EXPECT_THAT(rendered, HasSubstr("'hashed_selector'"));
    EXPECT_THAT(rendered, HasSubstr("'hashed_selector',\n            1,"));
    EXPECT_THAT(rendered, HasSubstr("'hashed_selector',\n            2,"));
    std::filesystem::remove(generatedFile);
}

}  // namespace P4::P4Tools::Test
