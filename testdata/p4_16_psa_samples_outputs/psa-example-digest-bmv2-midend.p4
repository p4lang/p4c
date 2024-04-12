#include <core.p4>
#include <bmv2/psa.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    bit<16>    type;
}

struct empty_metadata_t {
}

struct mac_learn_digest_t {
    bit<48> srcAddr;
    bit<32> ingress_port;
}

struct metadata {
    bool    _send_mac_learn_msg0;
    bit<48> _mac_learn_msg_srcAddr1;
    bit<32> _mac_learn_msg_ingress_port2;
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        parsed_hdr.ethernet.setInvalid();
        parsed_hdr.ipv4.setInvalid();
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4;
            default: start_0;
        }
    }
    state CommonParser_parse_ipv4 {
        buffer.extract<ipv4_t>(parsed_hdr.ipv4);
        transition start_0;
    }
    state start_0 {
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        parsed_hdr.ethernet.setInvalid();
        parsed_hdr.ipv4.setInvalid();
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4_0;
            default: start_1;
        }
    }
    state CommonParser_parse_ipv4_0 {
        buffer.extract<ipv4_t>(parsed_hdr.ipv4);
        transition start_1;
    }
    state start_1 {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.unknown_source") action unknown_source() {
        meta._send_mac_learn_msg0 = true;
        meta._mac_learn_msg_srcAddr1 = hdr.ethernet.srcAddr;
        meta._mac_learn_msg_ingress_port2 = istd.ingress_port;
    }
    @name("ingress.learned_sources") table learned_sources_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            unknown_source();
        }
        default_action = unknown_source();
    }
    @name("ingress.do_L2_forward") action do_L2_forward(@name("egress_port") bit<32> egress_port_3) {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = egress_port_3;
    }
    @name("ingress.do_tst") action do_tst(@name("egress_port") bit<32> egress_port_4, @name("serEnumT") bit<16> serEnumT) {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = egress_port_4;
    }
    @name("ingress.l2_tbl") table l2_tbl_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr");
        }
        actions = {
            do_L2_forward();
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("ingress.tst_tbl") table tst_tbl_0 {
        key = {
            meta._mac_learn_msg_ingress_port2: exact @name("meta.mac_learn_msg.ingress_port");
        }
        actions = {
            do_tst();
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action psaexampledigestbmv2l197() {
        meta._send_mac_learn_msg0 = false;
    }
    @hidden table tbl_psaexampledigestbmv2l197 {
        actions = {
            psaexampledigestbmv2l197();
        }
        const default_action = psaexampledigestbmv2l197();
    }
    apply {
        tbl_psaexampledigestbmv2l197.apply();
        learned_sources_0.apply();
        l2_tbl_0.apply();
        tst_tbl_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    @name("IngressDeparserImpl.mac_learn_digest") Digest<mac_learn_digest_t>() mac_learn_digest_0;
    @hidden action psaexampledigestbmv2l236() {
        mac_learn_digest_0.pack((mac_learn_digest_t){srcAddr = meta._mac_learn_msg_srcAddr1,ingress_port = meta._mac_learn_msg_ingress_port2});
    }
    @hidden action psaexampledigestbmv2l218() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_psaexampledigestbmv2l236 {
        actions = {
            psaexampledigestbmv2l236();
        }
        const default_action = psaexampledigestbmv2l236();
    }
    @hidden table tbl_psaexampledigestbmv2l218 {
        actions = {
            psaexampledigestbmv2l218();
        }
        const default_action = psaexampledigestbmv2l218();
    }
    apply {
        if (meta._send_mac_learn_msg0) {
            tbl_psaexampledigestbmv2l236.apply();
        }
        tbl_psaexampledigestbmv2l218.apply();
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaexampledigestbmv2l218_0() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_psaexampledigestbmv2l218_0 {
        actions = {
            psaexampledigestbmv2l218_0();
        }
        const default_action = psaexampledigestbmv2l218_0();
    }
    apply {
        tbl_psaexampledigestbmv2l218_0.apply();
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
