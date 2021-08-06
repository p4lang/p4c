/* Copyright 2021 SYRMIA LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Dusan Krdzic (dusan.krdzic@syrmia.com)
 *
 */

#include <gtest/gtest.h>

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/phv_source.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/extern.h>

using namespace bm;

extern int import_internet_checksum();

/* Frame (34 bytes) */
static const unsigned char raw_pkt[34] = {
    0x52, 0x54, 0x00, 0x12, 0x35, 0x02, 0x08, 0x00,
    0x27, 0x01, 0x8b, 0xbc, 0x08, 0x00, 0x00, 0xFE,
    0xC5, 0x23, 0xFD, 0xA1, 0xD6, 0x8A, 0xAF, 0x02,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x02
};

class PSA_InternetChecksumTest : public ::testing::Test {
 protected:
    HeaderType ethernetHeaderType, ipv4HeaderType;
    HeaderType metaHeaderType;
    ParseState ethernetParseState, ipv4ParseState;
    header_id_t ethernetHeader{0}, ipv4Header{1};
    header_id_t metaHeader{2};

    ErrorCodeMap error_codes;
    Parser parser;

    std::unique_ptr<PHVSourceIface> phv_source{nullptr};

    std::unique_ptr<bm::ExternType> instance{nullptr};

    PHVFactory phv_factory;

    std::unique_ptr<Packet> packet{nullptr};

    PSA_InternetChecksumTest()
        :   ethernetHeaderType("ethernet_t", 0), ipv4HeaderType("ipv4_t", 1),
            metaHeaderType("meta_t", 2),
            ethernetParseState("parse_ethernet", 0),
            ipv4ParseState("parse_ipv4", 1),
            error_codes(ErrorCodeMap::make_with_core()),
            parser("test_parser", 0, &error_codes),
            phv_source(PHVSourceIface::make_phv_source()),
            instance(ExternFactoryMap::get_instance()->
                    get_extern_instance("InternetChecksum")) {
        ethernetHeaderType.push_back_field("dstAddr", 48);
        ethernetHeaderType.push_back_field("srcAddr", 48);
        ethernetHeaderType.push_back_field("ethertype", 16);

        ipv4HeaderType.push_back_field("version", 4);
        ipv4HeaderType.push_back_field("ihl", 4);
        ipv4HeaderType.push_back_field("diffserv", 8);
        ipv4HeaderType.push_back_field("len", 16);
        ipv4HeaderType.push_back_field("id", 16);
        ipv4HeaderType.push_back_field("flags", 3);
        ipv4HeaderType.push_back_field("flagOffset", 13);
        ipv4HeaderType.push_back_field("ttl", 8);
        ipv4HeaderType.push_back_field("protocol", 8);
        ipv4HeaderType.push_back_field("checksum", 16);
        ipv4HeaderType.push_back_field("srcAddr", 32);
        ipv4HeaderType.push_back_field("dstAddr", 32);

        metaHeaderType.push_back_field("tmp", 16);

        phv_factory.push_back_header("ethernet", ethernetHeader,
                                    ethernetHeaderType);
        phv_factory.push_back_header("ipv4", ipv4Header, ipv4HeaderType);
        phv_factory.push_back_header("meta", metaHeader, metaHeaderType, true);
    }

    virtual void SetUp() {
        phv_source->set_phv_factory(0, &phv_factory);

        ParseSwitchKeyBuilder ethernetKeyBuilder;
        ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16);  // ethertype
        ethernetParseState.set_key_builder(ethernetKeyBuilder);

        ParseSwitchKeyBuilder ipv4KeyBuilder;
        ipv4KeyBuilder.push_back_field(ipv4Header, 8, 8);  // protocol
        ipv4ParseState.set_key_builder(ipv4KeyBuilder);

        ethernetParseState.add_extract(ethernetHeader);
        ipv4ParseState.add_extract(ipv4Header);

        char ethernet_ipv4_key[2];
        ethernet_ipv4_key[0] = 0x08;
        ethernet_ipv4_key[1] = 0x00;
        ethernetParseState.add_switch_case(sizeof(ethernet_ipv4_key),
                                        ethernet_ipv4_key, &ipv4ParseState);

        parser.set_init_state(&ethernetParseState);

        packet = std::unique_ptr<Packet>(new Packet(Packet::make_new(
                                        sizeof(raw_pkt),
                                        PacketBuffer(34, (const char *) raw_pkt,
                                        sizeof(raw_pkt)), phv_source.get())));
        parser.parse(packet.get());

        import_internet_checksum();
    }

    virtual void TearDown() { }
};

static std::unique_ptr<ActionPrimitive_> get_extern_primitive(
    const std::string &extern_name, const std::string &method_name) {
    return ActionOpcodesMap::get_instance()->get_primitive(
        "_" + extern_name + "_" + method_name);
}

TEST_F(PSA_InternetChecksumTest, PSA_InternetChecksumMethods) {
    uint16_t cksum;
    uint16_t sum;
    auto phv = packet.get()->get_phv();

    // ACTION ADD
    ActionFn actionFn_add("_InternetChecksum_add", 0, 0);
    ActionFnEntry actionFnEntry_add(&actionFn_add);
    auto primitive_add = get_extern_primitive("InternetChecksum", "add");
    ASSERT_NE(nullptr, primitive_add);
    actionFn_add.push_back_primitive(primitive_add.get());
    actionFn_add.parameter_push_back_extern_instance(instance.get());
    actionFn_add.parameter_start_field_list();
    actionFn_add.parameter_push_back_field(ipv4Header, 0);
    actionFn_add.parameter_push_back_field(ipv4Header, 1);
    actionFn_add.parameter_push_back_field(ipv4Header, 2);
    actionFn_add.parameter_push_back_field(ipv4Header, 3);
    actionFn_add.parameter_push_back_field(ipv4Header, 4);
    actionFn_add.parameter_push_back_field(ipv4Header, 5);
    actionFn_add.parameter_push_back_field(ipv4Header, 6);
    actionFn_add.parameter_push_back_field(ipv4Header, 7);
    actionFn_add.parameter_push_back_field(ipv4Header, 8);
    actionFn_add.parameter_push_back_field(ipv4Header, 10);
    actionFn_add.parameter_push_back_field(ipv4Header, 11);
    actionFn_add.parameter_end_field_list();

    // ACTION GET
    ActionFn actionFn_get("_InternetChecksum_get", 0, 0);
    ActionFnEntry actionFnEntry_get(&actionFn_get);
    auto primitive_get = get_extern_primitive("InternetChecksum", "get");
    ASSERT_NE(nullptr, primitive_get);
    actionFn_get.push_back_primitive(primitive_get.get());
    actionFn_get.parameter_push_back_extern_instance(instance.get());
    actionFn_get.parameter_push_back_field(ipv4Header, 9);

    // ACTION SUBTRACT
    ActionFn actionFn_sub("_InternetChecksum_subtract", 0, 0);
    ActionFnEntry actionFnEntry_sub(&actionFn_sub);
    auto primitive_sub = get_extern_primitive("InternetChecksum", "subtract");
    ASSERT_NE(nullptr, primitive_sub);
    actionFn_sub.push_back_primitive(primitive_sub.get());
    actionFn_sub.parameter_push_back_extern_instance(instance.get());
    actionFn_sub.parameter_start_field_list();
    actionFn_sub.parameter_push_back_field(ipv4Header, 10);
    actionFn_sub.parameter_push_back_field(ipv4Header, 11);
    actionFn_sub.parameter_end_field_list();

    // ACTION GET_STATE
    ActionFn actionFn_get_state("_InternetChecksum_get_state", 0, 0);
    ActionFnEntry actionFnEntry_get_state(&actionFn_get_state);
    auto primitive_get_state = get_extern_primitive("InternetChecksum", "get_state");
    ASSERT_NE(nullptr, primitive_get_state);
    actionFn_get_state.push_back_primitive(primitive_get_state.get());
    actionFn_get_state.parameter_push_back_extern_instance(instance.get());
    actionFn_get_state.parameter_push_back_field(metaHeader, 0);

    // ACTION SET_STATE
    ActionFn actionFn_set_state("_InternetChecksum_set_state", 0, 0);
    ActionFnEntry actionFnEntry_set_state(&actionFn_set_state);
    auto primitive_set_state = get_extern_primitive("InternetChecksum", "set_state");
    ASSERT_NE(nullptr, primitive_set_state);
    actionFn_set_state.push_back_primitive(primitive_set_state.get());
    actionFn_set_state.parameter_push_back_extern_instance(instance.get());
    actionFn_set_state.parameter_push_back_const(Data("0x01"));

    cksum = 0xb6ab;
    sum = 0x4954;

    actionFnEntry_add(packet.get());
    actionFnEntry_get(packet.get());
    actionFnEntry_get_state(packet.get());
    ASSERT_EQ(sum, phv->get_field("meta.tmp").get<uint16_t>());
    ASSERT_EQ(cksum, phv->get_field("ipv4.checksum").get<uint16_t>());

    cksum = 0xb6ae;
    sum =  0x4951;

    actionFnEntry_sub(packet.get());
    actionFnEntry_get(packet.get());
    actionFnEntry_get_state(packet.get());
    ASSERT_EQ(sum, phv->get_field("meta.tmp").get<uint16_t>());
    ASSERT_EQ(cksum, phv->get_field("ipv4.checksum").get<uint16_t>());

    sum = 0x01;

    actionFnEntry_set_state(packet.get());
    actionFnEntry_get_state(packet.get());
    ASSERT_EQ(sum, phv->get_field("meta.tmp").get<uint16_t>());
}
