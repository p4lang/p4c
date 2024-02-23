#include "backends/p4tools/modules/testgen/targets/bmv2/test/test_backend/stf.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <vector>

#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"

namespace Test {

using ::testing::HasSubstr;

TableConfig STFTest::getForwardTableConfig() {
    ActionArg port(new IR::Parameter("port", IR::Direction::None, IR::getBitType(9)),
                   IR::getConstant(IR::getBitType(9), big_int("0x2")));
    std::vector<ActionArg> args({port});
    const auto hit = ActionCall(
        "SwitchIngress.hit",
        new IR::P4Action("dummy", new IR::ParameterList({}, {}), new IR::BlockStatement()), args);

    auto const *exactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::getConstant(IR::getBitType(48), big_int("0x222222222222")));
    auto matches = TableMatchMap({{"hdr.ethernet.dst_addr", exactMatch}});

    return TableConfig(new IR::P4Table("table", new IR::TableProperties()),
                       {TableRule(matches, TestSpec::NO_PRIORITY, hit, 0)});
}

TableConfig STFTest::getIPRouteTableConfig() {
    const auto srcAddr =
        ActionArg(new IR::Parameter("srcAddr", IR::Direction::None, IR::getBitType(32)),
                  IR::getConstant(IR::getBitType(32), big_int("0xF81096F")));
    const auto dstAddr =
        ActionArg(new IR::Parameter("dstAddr", IR::Direction::None, IR::getBitType(48)),
                  IR::getConstant(IR::getBitType(48), big_int("0x12176CD3")));
    const auto dstPort =
        ActionArg(new IR::Parameter("dst_port", IR::Direction::None, IR::getBitType(9)),
                  IR::getConstant(IR::getBitType(9), big_int("0x2")));
    const auto args = std::vector<ActionArg>({srcAddr, dstAddr, dstPort});
    const auto nat = ActionCall(
        "SwitchIngress.nat",
        new IR::P4Action("dummy", new IR::ParameterList({}, {}), new IR::BlockStatement()), args);

    const auto *dipExactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::getConstant(IR::getBitType(32), big_int("0xA612895D")));

    const auto *vrfExactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::getConstant(IR::getBitType(16), big_int("0x0")));

    TableMatchMap matches({{"vrf", vrfExactMatch}, {"hdr.ipv4.dst_addr", dipExactMatch}});

    return TableConfig(new IR::P4Table("table", new IR::TableProperties()),
                       {TableRule(matches, TestSpec::NO_PRIORITY, nat, 0)});
}

/// Create a test spec with an Exact match and print an stf test.
TEST_F(STFTest, Stf01) {
    const auto *pld = IR::getConstant(
        IR::getBitType(512), big_int("0x22222222222200060708090a080045000056000100004006f94dc0a8"
                                     "0001c0a8000204d200500000000000000000500220000d2c0000000102"
                                     "03040506070809"));
    const auto *pldIgnMask = IR::getConstant(
        IR::getBitType(512), big_int("0xf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f01"
                                     "0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f010"
                                     "f0f0f0f0f0f0f0f0f0f0f0f0f0f0"));
    const auto ingressPacket = Packet(1, pld, pldIgnMask);
    const auto egressPacket = Packet(2, pld, pldIgnMask);

    const auto fwdConfig = getForwardTableConfig();

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables", "SwitchIngress.forward", &fwdConfig);

    TestBackendConfiguration testBackendConfiguration{"test01", 1, "test01", 1};
    auto testWriter = STF(testBackendConfiguration);
    testWriter.writeTestToFile(&testSpec, "", 1, 0);
}

/// Create a test spec with two Exact matches and print an stf test.
TEST_F(STFTest, Stf02) {
    /// TODO: If payload starts with leading 0s, they are truncated causing stf to fail with
    /// malformed packet, need to pad to account for leading zeros.
    const auto *pldIngress = IR::getConstant(
        IR::getBitType(512), big_int("0x22210203040500060708090a0800450000560001000040068"
                                     "a88c0a80001a612895d04d20050000000000000000050022000"
                                     "0000000000010203040506070809"));
    const auto *pldEgress = IR::getConstant(
        IR::getBitType(512), big_int("0x22210203040500060708090a080045000056000100004006e"
                                     "2c70f81096f12176cd304d20050000000000000000050022000"
                                     "0000000000010203040506070809"));
    const auto *pldIgnMask = IR::getConstant(
        IR::getBitType(512), big_int("0xf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f01"
                                     "0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f010"
                                     "f0f0f0f0f0f0f0f0f0f0f0f0f0f0"));
    const auto ingressPacket = Packet(5, pldIngress, pldIgnMask);
    const auto egressPacket = Packet(2, pldEgress, pldIgnMask);

    const auto fwdConfig = getForwardTableConfig();
    const auto ipRouteConfig = getIPRouteTableConfig();

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables", "SwitchIngress.forward", &fwdConfig);
    testSpec.addTestObject("tables", "SwitchIngress.ipRoute", &ipRouteConfig);

    TestBackendConfiguration testBackendConfiguration{"test02", 1, "test02", 2};
    auto testWriter = STF(testBackendConfiguration);
    testWriter.writeTestToFile(&testSpec, "", 2, 0);
}

TableConfig STFTest::gettest1TableConfig() {
    const auto val = ActionArg(new IR::Parameter("val", IR::Direction::None, IR::getBitType(8)),
                               IR::getConstant(IR::getBitType(8), big_int("0x7F")));
    const auto port = ActionArg(new IR::Parameter("port", IR::Direction::None, IR::getBitType(9)),
                                IR::getConstant(IR::getBitType(9), big_int("0x2")));
    const auto args = std::vector<ActionArg>({val, port});
    const auto setb1 = ActionCall(
        "setb1", new IR::P4Action("dummy", new IR::ParameterList({}, {}), new IR::BlockStatement()),
        args);

    const auto *f1TernaryMatch = new Ternary(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("ternary")),
        IR::getConstant(IR::getBitType(32), big_int("0xABCD0101")),
        IR::getConstant(IR::getBitType(32), big_int("0xFFFF0000")));

    TableMatchMap matches({{"data.f1", f1TernaryMatch}});

    return TableConfig(new IR::P4Table("table", new IR::TableProperties()),
                       {TableRule(matches, TestSpec::LOW_PRIORITY, setb1, 0)});
}

/// Create a test spec with a Ternary match and print an stf test.
TEST_F(STFTest, Stf03) {
    const auto *pldIngress =
        IR::getConstant(IR::getBitType(112), big_int("0x0000010100000202030355667788"));
    const auto *pldIngIgnMask =
        IR::getConstant(IR::getBitType(112), big_int("0x0000000000000000000000000000"));
    const auto *pldEgress =
        IR::getConstant(IR::getBitType(96), big_int("0x00000101deadd00dbeef7f66"));
    const auto *pldEgIgnMask =
        IR::getConstant(IR::getBitType(96), big_int("0x00000000ffffffffffff0000"));

    const auto ingressPacket = Packet(0, pldIngress, pldIngIgnMask);
    const auto egressPacket = Packet(2, pldEgress, pldEgIgnMask);

    const auto test1Config = gettest1TableConfig();

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables", "test1", &test1Config);

    TestBackendConfiguration testBackendConfiguration{"test03", 1, "test03", 3};
    auto testWriter = STF(testBackendConfiguration);
    try {
        testWriter.writeTestToFile(&testSpec, "", 3, 0);
    } catch (const Util::CompilerBug &e) {
        EXPECT_THAT(e.what(), HasSubstr("Unimplemented for Ternary FieldMatch"));
    }
}

TableConfig STFTest::gettest1TableConfig2() {
    const auto val = ActionArg(new IR::Parameter("val", IR::Direction::None, IR::getBitType(8)),
                               IR::getConstant(IR::getBitType(8), big_int("0x7F")));
    const auto port = ActionArg(new IR::Parameter("port", IR::Direction::None, IR::getBitType(9)),
                                IR::getConstant(IR::getBitType(9), big_int("0x2")));
    const auto args = std::vector<ActionArg>({val, port});
    const auto setb1 = ActionCall(
        "setb1", new IR::P4Action("dummy", new IR::ParameterList({}, {}), new IR::BlockStatement()),
        args);

    const auto *h1ExactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::getConstant(IR::getBitType(16), big_int("0x3")));

    const auto *f1TernaryMatch = new Ternary(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("ternary")),
        IR::getConstant(IR::getBitType(32), big_int("0xABCD0101")),
        IR::getConstant(IR::getBitType(32), big_int("0xFFFF0000")));

    const auto *b1ExactMatch = new Exact(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("exact")),
        IR::getConstant(IR::getBitType(8), big_int("0x1")));

    const auto *f2TernaryMatch = new Ternary(
        new IR::KeyElement(new IR::PathExpression("key"), new IR::PathExpression("ternary")),
        IR::getConstant(IR::getBitType(32), big_int("0xABCDD00D")),
        IR::getConstant(IR::getBitType(32), big_int("0x0000FFFF")));

    TableMatchMap matches1({{"data.f1", f1TernaryMatch}});

    TableMatchMap matches2({{"data.h1", h1ExactMatch},
                            {"data.f1", f1TernaryMatch},
                            {"data.b1", b1ExactMatch},
                            {"data.f2", f2TernaryMatch}});

    return TableConfig(new IR::P4Table("table", new IR::TableProperties()),
                       {TableRule(matches1, TestSpec::LOW_PRIORITY, setb1, 0),
                        TableRule(matches2, TestSpec::HIGH_PRIORITY, setb1, 0)});
}

/// Create a test spec with one Exact match and one Ternary match and print an stf test.
TEST_F(STFTest, Stf04) {
    const auto *pldIngress =
        IR::getConstant(IR::getBitType(112), big_int("0x0000010100000202030355667788"));
    const auto *pldIngIgnMask =
        IR::getConstant(IR::getBitType(112), big_int("0x0000000000000000000000000000"));
    const auto *pldEgress =
        IR::getConstant(IR::getBitType(96), big_int("0x00000101deadd00dbeef7f66"));
    const auto *pldEgIgnMask =
        IR::getConstant(IR::getBitType(96), big_int("0x00000000ffffffffffff0000"));

    const auto ingressPacket = Packet(0, pldIngress, pldIngIgnMask);
    const auto egressPacket = Packet(2, pldEgress, pldEgIgnMask);

    const auto test1Config = gettest1TableConfig2();

    auto testSpec = TestSpec(ingressPacket, egressPacket, {});
    testSpec.addTestObject("tables", "test1", &test1Config);

    TestBackendConfiguration testBackendConfiguration{"test04", 1, "test04", 4};
    auto testWriter = STF(testBackendConfiguration);
    try {
        testWriter.writeTestToFile(&testSpec, "", 4, 0);
    } catch (const Util::CompilerBug &e) {
        EXPECT_THAT(e.what(), HasSubstr("Unimplemented for Ternary FieldMatch"));
    }
}

}  // namespace Test
