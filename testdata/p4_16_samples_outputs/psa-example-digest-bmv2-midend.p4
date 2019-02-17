#include <core.p4>
#include <psa.p4>

typedef bit<48> EthernetAddress;
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

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct empty_metadata_t {
}

struct mac_learn_digest_t {
    EthernetAddress srcAddr;
    PortId_t        ingress_port;
}

struct metadata {
    bool    _send_mac_learn_msg0;
    bit<48> _mac_learn_msg_srcAddr1;
    bit<32> _mac_learn_msg_ingress_port2;
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    ethernet_t parsed_hdr_0_ethernet;
    ipv4_t parsed_hdr_0_ipv4;
    bool meta_1_send_mac_learn_msg;
    mac_learn_digest_t meta_1_mac_learn_msg;
    state start {
        parsed_hdr_0_ethernet.setInvalid();
        parsed_hdr_0_ipv4.setInvalid();
        meta_1_send_mac_learn_msg = meta._send_mac_learn_msg0;
        meta_1_mac_learn_msg.srcAddr = meta._mac_learn_msg_srcAddr1;
        meta_1_mac_learn_msg.ingress_port = meta._mac_learn_msg_ingress_port2;
        buffer.extract<ethernet_t>(parsed_hdr_0_ethernet);
        transition select(parsed_hdr_0_ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4;
            default: start_0;
        }
    }
    state CommonParser_parse_ipv4 {
        buffer.extract<ipv4_t>(parsed_hdr_0_ipv4);
        transition start_0;
    }
    state start_0 {
        parsed_hdr.ethernet = parsed_hdr_0_ethernet;
        parsed_hdr.ipv4 = parsed_hdr_0_ipv4;
        meta._send_mac_learn_msg0 = meta_1_send_mac_learn_msg;
        meta._mac_learn_msg_srcAddr1 = meta_1_mac_learn_msg.srcAddr;
        meta._mac_learn_msg_ingress_port2 = meta_1_mac_learn_msg.ingress_port;
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    ethernet_t parsed_hdr_1_ethernet;
    ipv4_t parsed_hdr_1_ipv4;
    bool meta_2_send_mac_learn_msg;
    mac_learn_digest_t meta_2_mac_learn_msg;
    state start {
        parsed_hdr_1_ethernet.setInvalid();
        parsed_hdr_1_ipv4.setInvalid();
        meta_2_send_mac_learn_msg = meta._send_mac_learn_msg0;
        meta_2_mac_learn_msg.srcAddr = meta._mac_learn_msg_srcAddr1;
        meta_2_mac_learn_msg.ingress_port = meta._mac_learn_msg_ingress_port2;
        buffer.extract<ethernet_t>(parsed_hdr_1_ethernet);
        transition select(parsed_hdr_1_ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4_0;
            default: start_1;
        }
    }
    state CommonParser_parse_ipv4_0 {
        buffer.extract<ipv4_t>(parsed_hdr_1_ipv4);
        transition start_1;
    }
    state start_1 {
        parsed_hdr.ethernet = parsed_hdr_1_ethernet;
        parsed_hdr.ipv4 = parsed_hdr_1_ipv4;
        meta._send_mac_learn_msg0 = meta_2_send_mac_learn_msg;
        meta._mac_learn_msg_srcAddr1 = meta_2_mac_learn_msg.srcAddr;
        meta._mac_learn_msg_ingress_port2 = meta_2_mac_learn_msg.ingress_port;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.unknown_source") action unknown_source() {
        meta._send_mac_learn_msg0 = true;
        meta._mac_learn_msg_srcAddr1 = hdr.ethernet.srcAddr;
        meta._mac_learn_msg_ingress_port2 = istd.ingress_port;
    }
    @name("ingress.learned_sources") table learned_sources_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        actions = {
            NoAction_0();
            unknown_source();
        }
        default_action = unknown_source();
    }
    @name("ingress.do_L2_forward") action do_L2_forward(PortId_t egress_port) {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = egress_port;
    }
    @name("ingress.l2_tbl") table l2_tbl_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        actions = {
            do_L2_forward();
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action act() {
        meta._send_mac_learn_msg0 = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        learned_sources_0.apply();
        l2_tbl_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    @name("IngressDeparserImpl.mac_learn_digest") Digest<mac_learn_digest_t>() mac_learn_digest_0;
    @hidden action act_0() {
        mac_learn_digest_0.pack({meta._mac_learn_msg_srcAddr1,meta._mac_learn_msg_ingress_port2});
    }
    @hidden action act_1() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        if (meta._send_mac_learn_msg0) 
            tbl_act_0.apply();
        tbl_act_1.apply();
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action act_2() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tbl_act_2.apply();
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

