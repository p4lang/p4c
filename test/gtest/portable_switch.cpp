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

#include <boost/algorithm/string/replace.hpp>
#include <boost/optional.hpp>
#include <type_traits>
#include "gtest/gtest.h"

#include "ir/ir.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "test/gtest/helpers.h"

namespace Test {

class PortableSwitchTest : public P4CTest {};

struct TestArgs {
    std::string ingressDecl = "";
    std::string ingressApply = "";
    std::string ingressMeta = "";
    std::string resubMeta = "";
    std::string recircMeta = "";
    std::string mirrorIngressMeta = "";
    std::string mirrorEgressMeta = "";
    std::string ingressParserDecl = "";
    std::string egressParserDecl = "";
    std::string ingressDeparserApply = "";
    std::string egressDeparserApply = "";
    std::string userDefinedStruct = "";
};

template<class T>
boost::optional<T>
createPSATest(TestArgs& args) {
    auto source = P4_SOURCE(P4Headers::PSA, R"(
        @diagnostic("uninitialized_out_param", "disable")
        typedef bit<48>  EthernetAddress;

        header ethernet_t {
            EthernetAddress dstAddr;
            EthernetAddress srcAddr;
            bit<16>         etherType;
        }

        header ipv4_t {
            bit<4>  version;
            bit<4>  ihl;
            bit<8>  diffserv;
            bit<16> totalLen;
            bit<16> identification;
            bit<3>  flags;
            bit<13> fragOffset;
            bit<8>  ttl;
            bit<8>  protocol;
            bit<16> hdrChecksum;
            bit<32> srcAddr;
            bit<32> dstAddr;
        }

        header data_t {
            bit<16> h1;
            bit<16> h2;
            bit<16> h3;
%INGRESS_METADATA%
        }

        typedef bit<8> clone_i2e_format_t;

%USER_DEFINED_DATA%

        struct fwd_metadata_t {
            bit<9> output_port;
        }

        struct resubmit_metadata_t {
%RESUBMIT_DATA%
        }

        struct recirculate_metadata_t {
%RECIRC_DATA%
        }

        struct clone_i2e_metadata_t {
%CLONE_I2E_DATA%
        }

        struct clone_e2e_metadata_t {
%CLONE_E2E_DATA%
        }

        struct headers {
            ethernet_t ethernet;
            ipv4_t ipv4;
        }

        struct metadata {
            data_t data;
            resubmit_metadata_t resubmit_meta;
            recirculate_metadata_t recirc_meta;
            clone_i2e_metadata_t clone_i2e_meta;
            clone_e2e_metadata_t clone_e2e_meta;
            fwd_metadata_t fwd_meta;
        }

        error {
            UnknownCloneI2EFormatId
        }

        parser IngressParserImpl(
            packet_in buffer,
            out headers parsed_hdr,
            inout metadata user_meta,
            in psa_ingress_parser_input_metadata_t istd,
            in resubmit_metadata_t resub_meta,
            in recirculate_metadata_t recirc_meta)
        {
%INGRESS_PARSER%
        }

        parser EgressParserImpl(
            packet_in buffer,
            out headers parsed_hdr,
            inout metadata meta,
            in psa_egress_parser_input_metadata_t istd,
            in metadata normal_meta,
            in clone_i2e_metadata_t clone_i2e_meta,
            in clone_e2e_metadata_t clone_e2e_meta)
        {
%EGRESS_PARSER%
        }

        control ingress(inout headers hdr,
                        inout metadata meta,
                        in    psa_ingress_input_metadata_t  istd,
                        inout psa_ingress_output_metadata_t ostd)
        {
%INGRESS_DECL%
            apply {
%INGRESS_APPLY%
            }
        }

        control egress(inout headers hdr,
                       inout metadata meta,
                       in    psa_egress_input_metadata_t  istd,
                       inout psa_egress_output_metadata_t ostd)
        {
            apply { }
        }

        control IngressDeparserImpl(
            packet_out packet,
            out clone_i2e_metadata_t clone_i2e_meta,
            out resubmit_metadata_t resub_meta,
            out metadata normal_meta,
            inout headers hdr,
            in metadata meta,
            in psa_ingress_output_metadata_t istd)
        {
            apply {
%INGRESS_DEPARSER_APPLY%
            }
        }

        control EgressDeparserImpl(
            packet_out packet,
            out clone_e2e_metadata_t clone_e2e_meta,
            out recirculate_metadata_t recirc_meta,
            inout headers hdr,
            in metadata meta,
            in psa_egress_output_metadata_t istd,
            in psa_egress_deparser_input_metadata_t edstd)
        {
            apply {
%EGRESS_DEPARSER_APPLY%
            }
        }

        IngressPipeline(IngressParserImpl(),
                        ingress(),
                        IngressDeparserImpl()) ip;

        EgressPipeline(EgressParserImpl(),
                       egress(),
                       EgressDeparserImpl()) ep;

        PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
    )");
    boost::replace_first(source, "%INGRESS_METADATA%", args.ingressMeta);
    boost::replace_first(source, "%INGRESS_DECL%", args.ingressDecl);
    boost::replace_first(source, "%INGRESS_APPLY%", args.ingressApply);
    boost::replace_first(source, "%RESUBMIT_DATA%", args.resubMeta);
    boost::replace_first(source, "%RECIRC_DATA%", args.recircMeta);
    boost::replace_first(source, "%CLONE_I2E_DATA%", args.mirrorIngressMeta);
    boost::replace_first(source, "%CLONE_E2E_DATA%", args.mirrorEgressMeta);
    boost::replace_first(source, "%INGRESS_PARSER%", args.ingressParserDecl);
    boost::replace_first(source, "%EGRESS_PARSER%", args.egressParserDecl);
    boost::replace_first(source, "%INGRESS_DEPARSER_APPLY%", args.ingressDeparserApply);
    boost::replace_first(source, "%EGRESS_DEPARSER_APPLY%", args.egressDeparserApply);
    boost::replace_first(source, "%USER_DEFINED_DATA%", args.userDefinedStruct);


    auto& options = P4CContext::get().options();
    options.target = "bmv2-psa-p4org";

    return T::create(source);
}

boost::optional<FrontendTestCase>
createPSAIngressTest(const std::string& ingressDecl,
                     const std::string& ingressApply,
                     const std::string& ingressMeta = "") {
    TestArgs args;
    args.ingressMeta = ingressMeta;
    args.ingressDecl = ingressDecl;
    args.ingressApply = ingressApply;
    args.ingressParserDecl = P4_SOURCE(R"(
            state start {
                buffer.extract(parsed_hdr.ethernet);
                buffer.extract(parsed_hdr.ipv4);
                transition accept;
            })");
    args.egressParserDecl= R"(
            state start {
                transition accept;
            })";
    return createPSATest<FrontendTestCase>(args);
}

// Counter can be instantiated in a control block;
// its count() method can be invoked in an action.
TEST_F(PortableSwitchTest, CounterInExactMatchTable) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Counter<bit<10>,bit<12>>(1024, CounterType_t.PACKETS) counter;
        action execute() {
          counter.count(1024);
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute(); }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));
    EXPECT_TRUE(test);
}

// If two counters are instantiated in a control block,
// Both can be invoked in the same action.
TEST_F(PortableSwitchTest, TwoCountersInSameAction) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Counter<bit<10>,bit<12>>(1024, CounterType_t.PACKETS) counter0;
        Counter<bit<10>,bit<12>>(1024, CounterType_t.PACKETS) counter1;
        action execute() {
          counter0.count(1024);
          counter1.count(1024);
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute(); }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));
    EXPECT_TRUE(test);
}

// A Counter can be invoked in the apply statement of a control block.
TEST_F(PortableSwitchTest, CounterInApplyBlock) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Counter<bit<10>,bit<12>>(1024, CounterType_t.PACKETS) counter0;
)"),
P4_SOURCE(R"(
        counter0.count(1024);
)"));

    EXPECT_TRUE(test);
}

// A DirectCounter can be attached to a table without an explicit invocation.
TEST_F(PortableSwitchTest, DirectCounterInTable) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        DirectCounter<bit<12>>(CounterType_t.PACKETS) counter0;

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; }
          psa_direct_counters = { counter0 };
        }
)"),
P4_SOURCE(R"(
         tbl.apply();
)"));

    // EXPECT_EQ(::diagnosticCount(), 0u);
    EXPECT_TRUE(test);
}

// A DirectCounter can be invoked in an action
TEST_F(PortableSwitchTest, DirectCounterInAction) {
    auto test = createPSAIngressTest(
            P4_SOURCE(R"(
        DirectCounter<bit<12>>(CounterType_t.PACKETS) counter0;
        DirectCounter<bit<12>>(CounterType_t.PACKETS) counter1;

        action execute() {
          counter0.count();
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute; }
          psa_direct_counters = { counter0, counter1 };
        }
)"),
            P4_SOURCE(R"(
         tbl.apply();
)"));

    EXPECT_TRUE(test);
}

// DirectCounter cannot be shared between tables,
// however, the check is implemented in the backend.
TEST_F(PortableSwitchTest, DirectCounterNoSharing) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        DirectCounter<bit<12>>(CounterType_t.PACKETS) counter0;

        action execute() {
          counter0.count();
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute; }
          psa_direct_counters = { counter0 };
        }

        table tbl2 {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute; }
          psa_direct_counters = { counter0 };
        }
)"),
P4_SOURCE(R"(
         tbl.apply();
         tbl2.apply();
)"));

    // EXPECT_EQ(::diagnosticCount(), 0u);
    EXPECT_TRUE(test);
}

// DirectCounter cannot be shared between tables,
// however, the check is implemented in the backend.
TEST_F(PortableSwitchTest, DirectCounterNoSharing) {
}

// MeterColor_t is not converted to bit<n>
TEST_F(PortableSwitchTest, DISABLED_MeterColorIsEnum) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Meter<bit<12>>(1024, MeterType_t.PACKETS) meter0;

        action execute(bit<12> index, MeterColor_t color) {
          meter0.execute(index, color);
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute; }
        }
)"),
P4_SOURCE(R"(
         tbl.apply();
)"));

    EXPECT_TRUE(test);
}

// Expressions whose type is an enum cannot be cast to or from any other type.
// Error message is:
//   cast: Illegal cast from MeterColor_t to bit<16>
TEST_F(PortableSwitchTest, MeterUnableToCastToBit) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Meter<bit<12>>(1024, MeterType_t.PACKETS) meter0;

        action execute(bit<12> index) {
          meta.data.h1 = (bit<16>)meter0.execute(index, MeterColor_t.GREEN);
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute; }
        }
)"),
P4_SOURCE(R"(
         tbl.apply();
)"));

    EXPECT_TRUE(test);
}

// Executing meter in the control flow is not supported (should be).
// Error message is:
//    Method Call meter0_0/meter0.execute(0); not apply
TEST_F(PortableSwitchTest, DISABLED_MeterColorCompareToEnum) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Meter<bit<12>>(1024, MeterType_t.PACKETS) meter0;

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; }
        }
)"),
P4_SOURCE(R"(
        if (meter0.execute(0) == MeterColor_t.GREEN) {
           tbl.apply();
        }
)"));

    EXPECT_TRUE(test);
}

/// Direct meter can be invoked by table without explicit call
/// to execute in action.
TEST_F(PortableSwitchTest, DirectMeterNoInvoke) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        DirectMeter(MeterType_t.PACKETS) meter0;

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; }
          psa_direct_meters = { meter0 };
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_TRUE(test);
}

/// Direct meter can be invoked in an action
TEST_F(PortableSwitchTest, DirectMeterInvokedInAction) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        DirectMeter(MeterType_t.PACKETS) meter0;

        action execute_meter () {
          meter0.execute();
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; }
          psa_direct_meters = { meter0 };
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_TRUE(test);
}

/// Direct meter cannot be invoked in the table that does not
/// own it.
TEST_F(PortableSwitchTest, DISABLED_DirectMeterInvokedInWrongTable) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        DirectMeter(MeterType_t.PACKETS) meter0;

        action execute_meter () {
          meter0.execute();
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; }
          psa_direct_meters = { meter0 };
        }

        table tbl2 {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute_meter; }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
        tbl2.apply();
)"));

    EXPECT_FALSE(test);
}

/// Registers are stateful memory that can be accessed in an action
/// Error message is:
///   Could not find declaration for Register
TEST_F(PortableSwitchTest, RegisterReadInAction) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Register<bit<10>, bit<10>>(1024) reg;

        action execute_register(bit<10> idx) {
          bit<10> data = reg.read(idx);
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute_register; }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_TRUE(test);
}

/// Register can be written in an action
TEST_F(PortableSwitchTest, RegisterWriteInAction) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Register<bit<16>, bit<10>>(1024) reg;

        action execute_register(bit<10> idx) {
          reg.write(idx, meta.data.h1);
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute_register; }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_FALSE(test);
}

/// Register access may be out-of-bound.
/// PSA specifies an out of bounds write has no effect on the state of the system.
TEST_F(PortableSwitchTest, RegisterOutOfBound) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Register<bit<16>, bit<10>>(512) reg;

        action execute_register(bit<10> idx) {
          reg.write(idx, meta.data.h1);
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute_register; }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_FALSE(test);
}

// An 'Random' extern generates a random number in an action
TEST_F(PortableSwitchTest, RandomInAction) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Random<bit<16>>(200, 400) rand;

        action execute_random() {
          meta.data.h1 = rand.read();
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; execute_random; }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_FALSE(test);
}

/// Action profile is used as an implementation to table.
TEST_F(PortableSwitchTest, ActionProfileInTable) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        ActionProfile(1024) ap;

        action a1() { }
        action a2() { }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; a1; a2; }
          psa_implementation = ap;
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_FALSE(test);
}

/// Multiple action profile in the same table is not supported
TEST_F(PortableSwitchTest, ActionProfileMultiple) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        ActionProfile(1024) ap;
        ActionProfile(1024) ap1;

        action a1() { }
        action a2() { }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; a1; a2; }
          psa_implementation = { ap, ap1 };
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_FALSE(test);
}

/// Action profile can be shared between tables if all tables
/// implements the same set of actions
TEST_F(PortableSwitchTest, ActionProfileInMultipleTablesWithSameSet) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        ActionProfile(1024) ap;

        action a1() { }
        action a2() { }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; a1; a2; }
          psa_implementation = ap;
        }

        table tbl2 {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; a1; a2; }
          psa_implementation = ap;
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
        tbl2.apply();
)"));

    EXPECT_FALSE(test);
}

/// Action profile cannot be shared between tables if the
/// tables implements a different set of actions
TEST_F(PortableSwitchTest, ActionProfileInMultipleTablesWithDiffSet) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        ActionProfile(1024) ap;

        action a1() { }
        action a2() { }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; a2; }
          psa_implementation = ap;
        }

        table tbl2 {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; a1; }
          psa_implementation = ap;
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
        tbl2.apply();
)"));

    EXPECT_FALSE(test);
}

/// action selector uses key:selector as selection bits
TEST_F(PortableSwitchTest, ActionSelectorInTableWithSingleKey) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        ActionSelector(HashAlgorithm_t.CRC32, 32w1024, 32w16) as;

        action a1() { }
        action a2() { }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
            meta.data.h1 : selector;
          }
          actions = { NoAction; a1; a2; }
          psa_implementation = as;
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_FALSE(test);
}

/// multiple selector key can be used in action selector
TEST_F(PortableSwitchTest, ActionSelectorInTableWithMultipleKeys) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        ActionSelector(HashAlgorithm_t.CRC32, 32w1024, 32w16) as;

        action a1() { }
        action a2() { }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
            meta.data.h1 : selector;
            meta.data.h2 : selector;
          }
          actions = { NoAction; a1; a2; }
          psa_implementation = as;
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_FALSE(test);
}

/// it is illegal to define 'selector' type match field if the
/// table does not have an action selector implementation.
TEST_F(PortableSwitchTest, ActionSelectorIllegalSelectorKey) {
    auto test = createPSAIngressTest(
P4_SOURCE(R"(
        action a1() { }
        action a2() { }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
            meta.data.h1 : selector;
            meta.data.h2 : selector;
          }
          actions = { NoAction; a1; a2; }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_TRUE(test);
}

// ParserValueSet

// Hash
TEST_F(PortableSwitchTest, HashInAction) {
     auto test = createPSAIngressTest(
P4_SOURCE(R"(
        Hash<bit<16>>(HashAlgorithm_t.CRC16) h;

        action a1() {
          meta.data.h1 = h.get_hash(hdr.ethernet);
        }

        table tbl {
          key = {
            hdr.ethernet.srcAddr : exact;
          }
          actions = { NoAction; a1; }
        }
)"),
P4_SOURCE(R"(
        tbl.apply();
)"));

    EXPECT_FALSE(test);
}

/////////////////////////////////////////////////////////////////////////////////////////

boost::optional<FrontendTestCase>
createPSAResubmitTest(const std::string &resubMeta,
                      const std::string &parserDecl,
                      const std::string &deparserApply) {
    TestArgs args;
    args.resubMeta = resubMeta;
    args.ingressParserDecl = parserDecl;
    args.ingressDeparserApply = deparserApply;
    args.egressParserDecl= R"(
            state start {
                transition accept;
            })";
    return createPSATest<FrontendTestCase>(args);
}

// base implementation of resubmit in psa, direct struct assignment
TEST_F(PortableSwitchTest, ResubmitHeaderAssignment) {
    auto test = createPSAResubmitTest(
P4_SOURCE(R"()"),
P4_SOURCE(R"(
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.RESUBMIT: copy_resubmit_meta;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_resubmit_meta {
          user_meta.resubmit_meta = resub_meta;
          transition packet_in_parsing;
        }

        state packet_in_parsing {
          transition accept;
        }
)"),
P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

// user may manually assign fields in resubmit metadata to fields in user metadata.
TEST_F(PortableSwitchTest, ResubmitFieldAssignment) {
    auto test = createPSAResubmitTest(
P4_SOURCE(R"(
        bit<8> d1;
        bit<8> d2;
)"),
P4_SOURCE(R"(
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.RESUBMIT: copy_resubmit_meta;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_resubmit_meta {
          user_meta.resubmit_meta.d1 = resub_meta.d1;
          user_meta.resubmit_meta.d2 = resub_meta.d2;
          transition packet_in_parsing;
        }

        state packet_in_parsing {
          transition accept;
        }
)"),
P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

// user may NOT reorder fields in resubmit metadata to fields in user metadata.
TEST_F(PortableSwitchTest, ResubmitReorderdFieldAssignment) {
    auto test = createPSAResubmitTest(
P4_SOURCE(R"(
        bit<8> d1;
        bit<8> d2;
)"),
P4_SOURCE(R"(
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.RESUBMIT: copy_resubmit_meta;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_resubmit_meta {
          user_meta.resubmit_meta.d1 = resub_meta.d2;
          user_meta.resubmit_meta.d2 = resub_meta.d1;
          transition packet_in_parsing;
        }

        state packet_in_parsing {
          transition accept;
        }
)"),
P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

// select on istd.packet_path does not have to be in the first state, but
// must be the first thing to check in the parse graph.
TEST_F(PortableSwitchTest, ResubmitEmptyFirstState) {
    auto test = createPSAResubmitTest(
P4_SOURCE(R"()"),
P4_SOURCE(R"(
        state start {
          transition __start;
        }

        state __start {
          transition select(istd.packet_path) {
            PacketPath_t.RESUBMIT: copy_resubmit_meta;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_resubmit_meta {
          user_meta.resubmit_meta = resub_meta;
          transition packet_in_parsing;
        }

        state packet_in_parsing {
          transition accept;
        }
)"),
P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

// however, if an extraction happens before checking the packet_path
// metadata, that is an program error.
TEST_F(PortableSwitchTest, ResubmitExtractBeforeCheckPacketPath) {
    auto test = createPSAResubmitTest(
P4_SOURCE(R"()"),
P4_SOURCE(R"(
        state start {
          buffer.extract(parsed_hdr.ethernet);
          transition __start;
        }
        state __start {
          transition select(istd.packet_path) {
            PacketPath_t.RESUBMIT: copy_resubmit_meta;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_resubmit_meta {
          user_meta.resubmit_meta = resub_meta;
          transition packet_in_parsing;
        }
        state packet_in_parsing {
          transition accept;
        }
)"),
P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

// user can extract resubmit_metadata in multiple parser states.
TEST_F(PortableSwitchTest, ResubmitMultipleExtractState) {
     auto test = createPSAResubmitTest(
P4_SOURCE(R"(
        bit<8> d1;
        bit<8> d2;
)"),
P4_SOURCE(R"(
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.RESUBMIT: copy_resubmit_meta_0;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_resubmit_meta_0 {
          user_meta.resubmit_meta.d1 = resub_meta.d1;
          transition copy_resubmit_data_1;
        }
        state copy_resubmit_data_1 {
          user_meta.resubmit_meta.d2 = resub_meta.d2;
          transition packet_in_parsing;
        }

        state packet_in_parsing {
          transition accept;
        }
)"),
P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

// user may copy part of the resubmit metadata, select on the copied
// value and branch to next parse state to continue copying resubmit
// metadata
TEST_F(PortableSwitchTest, ResubmitCopyAndSelectOnField) {
     auto test = createPSAResubmitTest(
P4_SOURCE(R"(
        bit<8> d1;
        bit<8> d2;
)"),
P4_SOURCE(R"(
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.RESUBMIT: copy_resubmit_meta_0;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_resubmit_meta_0 {
          user_meta.resubmit_meta.d1 = resub_meta.d1;
          transition select(user_meta.resubmit_meta.d1) {
             8w0 : resubmit_data_is_0;
             8w1 : resubmit_data_is_1;
          }
        }
        state resubmit_data_is_0 {
          user_meta.resubmit_meta.d2 = resub_meta.d2;
          transition packet_in_parsing;
        }
        state resubmit_data_is_1 {
          user_meta.resubmit_meta.d2 = resub_meta.d2;
          transition packet_in_parsing;
        }
        state packet_in_parsing {
          transition accept;
        }
)"),
P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

// user may directly select on the resubmit metadata and branch
// to subsequent parse state.
TEST_F(PortableSwitchTest, ResubmitSelectOnField) {
    auto test = createPSAResubmitTest(
P4_SOURCE(R"(
        bit<8> d1;
        bit<8> d2;
        bit<8> d3;
)"),
P4_SOURCE(R"(
        state copy_resubmit_meta_0 {
          transition select(resub_meta.d1) {
             8w0 : resubmit_data_is_0;
             8w1 : resubmit_data_is_1;
          }
        }
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.RESUBMIT: copy_resubmit_meta_0;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state resubmit_data_is_0 {
          user_meta.resubmit_meta.d2 = resub_meta.d3;
          user_meta.resubmit_meta.d3 = resub_meta.d2;
          transition packet_in_parsing;
        }
        state resubmit_data_is_1 {
          user_meta.resubmit_meta.d2 = resub_meta.d2;
          user_meta.resubmit_meta.d3 = resub_meta.d3;
          transition packet_in_parsing;
        }
        state packet_in_parsing {
          transition accept;
        }
)"),
P4_SOURCE(R"(
)"));

    EXPECT_TRUE(test);
}

// user may use a struct to implement a header union with a selector byte
// to choose sub-field in the struct.

// user may use a header union to support multiple resubmit data type.

/////////////////////////////////////////////////////////////////////////////////////////

boost::optional<FrontendTestCase>
createPSARecircTest(const std::string &recircMeta,
                    const std::string &parserDecl,
                    const std::string &deparserApply,
                    const std::string &userDefinedStruct="") {
    TestArgs args;
    args.recircMeta = recircMeta;
    args.ingressParserDecl = parserDecl;
    args.egressParserDecl = R"(
            state start {
                transition accept;
            })";
    args.egressDeparserApply = deparserApply;
    args.userDefinedStruct = userDefinedStruct;
    return createPSATest<FrontendTestCase>(args);
}

// the base use case for recirculate is to assign the recirculated
// header to user metadata.
TEST_F(PortableSwitchTest, RecirculateHeaderAssignment) {
    auto test = createPSARecircTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.RECIRCULATE: copy_recirculate_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_recirculate_meta {
      user_meta.recirc_meta = recirc_meta;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    })"),
        P4_SOURCE(R"(
    recirc_meta = meta.recirc_meta;
    )"));
    EXPECT_TRUE(test);
}

// A keen programmer can assign each field in the recirculated header
// to the corresponding fields in user metadata.
TEST_F(PortableSwitchTest, RecirculateFieldAssignment) {
    auto test = createPSARecircTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.RECIRCULATE: copy_recirc_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_recirc_meta {
      user_meta.recirc_meta.d1 = recirc_meta.d1;
      user_meta.recirc_meta.d2 = recirc_meta.d2;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    })"),
        P4_SOURCE(R"(
    recirc_meta = meta.recirc_meta;
    )"));
    EXPECT_TRUE(test);
}

// Using struct inside the recirculate metadata struct is not yet supported.
TEST_F(PortableSwitchTest, DISABLED_RecirculateNestedStruct) {
    auto test = createPSARecircTest(
        P4_SOURCE(R"(
    user_defined_t umd;
    bit<8> d2;)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.RECIRCULATE: copy_recirc_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_recirc_meta {
      user_meta.recirc_meta.umd.d1 = recirc_meta.umd.d1;
      user_meta.recirc_meta.d2 = recirc_meta.d2;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    })"),
        P4_SOURCE(R"(
    recirc_meta = meta.recirc_meta;
    )"),
        P4_SOURCE(R"(
    struct user_defined_t {
        bit<8> d1;
    })"));
    EXPECT_FALSE(test);
}

// the recirculate header fields can be assigned to a different
// header field in the user metadata.
TEST_F(PortableSwitchTest, RecirculateReorderFieldAssignment) {
    auto test = createPSARecircTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.RECIRCULATE: copy_recirc_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_recirc_meta {
      user_meta.recirc_meta.d1 = recirc_meta.d2;
      user_meta.recirc_meta.d2 = recirc_meta.d1;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    })"),
        P4_SOURCE(R"(
    recirc_meta = meta.recirc_meta;
    )"));
    EXPECT_TRUE(test);
}

// recirculate header fields can be read in multiple consecutive
// parse states if the programmer choose to write this way.
TEST_F(PortableSwitchTest, RecirculateMultipleExtractState) {
    auto test = createPSARecircTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.RECIRCULATE: copy_recirc_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_recirc_meta {
      user_meta.recirc_meta.d1 = recirc_meta.d1;
      transition copy_recirc_meta_1;
    }
    state copy_recirc_meta_1 {
      user_meta.recirc_meta.d2 = recirc_meta.d2;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    })"),
        P4_SOURCE(R"(
    recirc_meta = meta.recirc_meta;
    )"));
    EXPECT_TRUE(test);
}

// extract recirculate header from packet with emitting any
// recirculate header in deparser does not make any sense.
// the compiler should optimize away the extract state in parser.
TEST_F(PortableSwitchTest, RecirculateUnusedPacketPath) {
    auto test = createPSARecircTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.RECIRCULATE: copy_recirc_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_recirc_meta {
      user_meta.recirc_meta.d1 = recirc_meta.d2;
      user_meta.recirc_meta.d2 = recirc_meta.d1;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    })"),
        P4_SOURCE(R"()"));
    EXPECT_TRUE(test);
}

// if a user missed to assign value to some fields in the
// recirculate header, compiler should raise an error.
TEST_F(PortableSwitchTest, RecirculateWritePartialHeader) {
    auto test = createPSARecircTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.RECIRCULATE: copy_recirc_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_recirc_meta {
      user_meta.recirc_meta.d1 = recirc_meta.d1;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    })"),
        P4_SOURCE(R"(
    recirc_meta.d1 = meta.recirc_meta.d1;
    )"));
    EXPECT_TRUE(test);
}

// read from uninitialized header fields in recirculate header
// results in undefined value. The compiler should issue an
// warning/error in this case.
TEST_F(PortableSwitchTest, RecirculateReadPartialHeader) {
    auto test = createPSARecircTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.RECIRCULATE: copy_recirc_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_recirc_meta {
      user_meta.recirc_meta.d2 = recirc_meta.d2;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    })"),
        P4_SOURCE(R"(
    recirc_meta.d1 = meta.recirc_meta.d1;
    )"));
    EXPECT_TRUE(test);
}

/////////////////////////////////////////////////////////////////////////////////////////

boost::optional<FrontendTestCase>
createPSACloneI2eTest(const std::string &mirrorMeta,
                   const std::string &egressParserDecl,
                   const std::string &deparserApply) {
    TestArgs args;
    args.mirrorIngressMeta = mirrorMeta;
    args.ingressParserDecl = R"(state start { transition accept; })";
    args.egressParserDecl = egressParserDecl;
    args.ingressDeparserApply = deparserApply;
    args.userDefinedStruct = R"()";
    return createPSATest<FrontendTestCase>(args);
}

TEST_F(PortableSwitchTest, CloneAssignHeader) {
    auto test = createPSACloneI2eTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;
        )"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.CLONE_I2E: copy_clone_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_clone_meta {
      meta.clone_i2e_meta = clone_i2e_meta;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    }
        )"),
        P4_SOURCE(R"(
    clone_i2e_meta = meta.clone_i2e_meta;
        )"));
    EXPECT_TRUE(test);
}

TEST_F(PortableSwitchTest, CloneAssignFields) {
    auto test = createPSACloneI2eTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;
)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.CLONE_I2E: copy_clone_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_clone_meta {
      meta.clone_i2e_meta.d1 = clone_i2e_meta.d1;
      meta.clone_i2e_meta.d2 = clone_i2e_meta.d2;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    }
)"),
        P4_SOURCE(R"(
    clone_i2e_meta = meta.clone_i2e_meta;
)")
    );
    EXPECT_TRUE(test);
}

TEST_F(PortableSwitchTest, CloneReorderFields) {
    auto test = createPSACloneI2eTest(
        P4_SOURCE(R"(
    bit<8> d1;
    bit<8> d2;
)"),
        P4_SOURCE(R"(
    state start {
      transition select(istd.packet_path) {
        PacketPath_t.CLONE_I2E: copy_clone_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_clone_meta {
      meta.clone_i2e_meta.d1 = clone_i2e_meta.d2;
      meta.clone_i2e_meta.d2 = clone_i2e_meta.d1;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    }
)"),
        P4_SOURCE(R"(
    clone_i2e_meta = meta.clone_i2e_meta;
)")
    );
    EXPECT_TRUE(test);
}

TEST_F(PortableSwitchTest, CloneEmptyFirstState) {
    auto test = createPSACloneI2eTest(
        P4_SOURCE(R"()"),
        P4_SOURCE(R"(
    state start {
      transition __start;
    }
    state __start {
      transition select(istd.packet_path) {
        PacketPath_t.CLONE_I2E: copy_clone_meta;
        PacketPath_t.NORMAL: packet_in_parsing;
      }
    }
    state copy_clone_meta {
      meta.clone_i2e_meta = clone_i2e_meta;
      transition packet_in_parsing;
    }
    state packet_in_parsing {
      transition accept;
    }
)"),
        P4_SOURCE(R"(
    clone_i2e_meta = meta.clone_i2e_meta;
)")
    );
    EXPECT_TRUE(test);
}

TEST_F(PortableSwitchTest, DISABLED_CloneExtractBeforeCheckPacketPath) {
    auto test = createPSACloneI2eTest(
        P4_SOURCE(R"()"),
        P4_SOURCE(R"(
        state start {
          buffer.extract(parsed_hdr.ethernet);
          transition __start;
        }
        state __start {
          transition select(istd.packet_path) {
            PacketPath_t.CLONE_I2E: copy_clone_meta;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_clone_meta {
          meta.clone_i2e_meta = clone_i2e_meta;
          transition packet_in_parsing;
        }
        state packet_in_parsing {
          transition accept;
        }
)"),
        P4_SOURCE(R"()"));
    EXPECT_TRUE(test);
}

TEST_F(PortableSwitchTest, CloneMultipleExtractState) {
    auto test = createPSACloneI2eTest(
        P4_SOURCE(R"(
        bit<8> d1;
        bit<8> d2;
)"),
        P4_SOURCE(R"(
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.CLONE_I2E: copy_clone_meta_0;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_clone_meta_0 {
          meta.clone_i2e_meta.d1 = clone_i2e_meta.d1;
          transition copy_clone_meta_1;
        }
        state copy_clone_meta_1 {
          meta.clone_i2e_meta.d2 = clone_i2e_meta.d2;
          transition packet_in_parsing;
        }

        state packet_in_parsing {
          transition accept;
        }
)"),
        P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

TEST_F(PortableSwitchTest, CloneCopyAndSelectOnField) {
    auto test = createPSACloneI2eTest(
        P4_SOURCE(R"(
        bit<8> d1;
        bit<8> d2;
)"),
        P4_SOURCE(R"(
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.CLONE_I2E: copy_clone_meta_0;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state copy_clone_meta_0 {
          meta.clone_i2e_meta.d1 = clone_i2e_meta.d1;
          transition select(meta.clone_i2e_meta.d1) {
             8w0 : clone_data_is_0;
             8w1 : clone_data_is_1;
          }
        }
        state clone_data_is_0 {
          meta.clone_i2e_meta.d2 = clone_i2e_meta.d2;
          transition packet_in_parsing;
        }
        state clone_data_is_1 {
          meta.clone_i2e_meta.d2 = clone_i2e_meta.d2;
          transition packet_in_parsing;
        }
        state packet_in_parsing {
          transition accept;
        }
)"),
        P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

TEST_F(PortableSwitchTest, CloneSelectOnField) {
    auto test = createPSACloneI2eTest(
        P4_SOURCE(R"(
        bit<8> d1;
        bit<8> d2;
        bit<8> d3;
        )"),
        P4_SOURCE(R"(
        state copy_clone_meta_0 {
          transition select(clone_i2e_meta.d1) {
             8w0 : clone_data_is_0;
             8w1 : clone_data_is_1;
          }
        }
        state start {
          transition select(istd.packet_path) {
            PacketPath_t.CLONE_I2E: copy_clone_meta_0;
            PacketPath_t.NORMAL: packet_in_parsing;
          }
        }
        state clone_data_is_0 {
          meta.clone_i2e_meta.d2 = clone_i2e_meta.d3;
          meta.clone_i2e_meta.d3 = clone_i2e_meta.d2;
          transition packet_in_parsing;
        }
        state clone_data_is_1 {
          meta.clone_i2e_meta.d2 = clone_i2e_meta.d2;
          meta.clone_i2e_meta.d3 = clone_i2e_meta.d3;
          transition packet_in_parsing;
        }
        state packet_in_parsing {
          transition accept;
        }
        )"),
        P4_SOURCE(R"()"));

    EXPECT_TRUE(test);
}

}  // namespace Test
