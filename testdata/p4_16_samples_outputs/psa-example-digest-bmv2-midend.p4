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
    bool               send_mac_learn_msg;
    mac_learn_digest_t mac_learn_msg;
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    ethernet_t parsed_hdr_2_ethernet;
    ipv4_t parsed_hdr_2_ipv4;
    bool meta_0_send_mac_learn_msg;
    mac_learn_digest_t meta_0_mac_learn_msg;
    state start {
        parsed_hdr_2_ethernet.setInvalid();
        parsed_hdr_2_ipv4.setInvalid();
        meta_0_send_mac_learn_msg = meta.send_mac_learn_msg;
        meta_0_mac_learn_msg.srcAddr = meta.mac_learn_msg.srcAddr;
        meta_0_mac_learn_msg.ingress_port = meta.mac_learn_msg.ingress_port;
        buffer.extract<ethernet_t>(parsed_hdr_2_ethernet);
        transition select(parsed_hdr_2_ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4;
            default: start_0;
        }
    }
    state CommonParser_parse_ipv4 {
        buffer.extract<ipv4_t>(parsed_hdr_2_ipv4);
        transition start_0;
    }
    state start_0 {
        parsed_hdr.ethernet = parsed_hdr_2_ethernet;
        parsed_hdr.ipv4 = parsed_hdr_2_ipv4;
        meta.send_mac_learn_msg = meta_0_send_mac_learn_msg;
        meta.mac_learn_msg.srcAddr = meta_0_mac_learn_msg.srcAddr;
        meta.mac_learn_msg.ingress_port = meta_0_mac_learn_msg.ingress_port;
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    ethernet_t parsed_hdr_3_ethernet;
    ipv4_t parsed_hdr_3_ipv4;
    bool meta_4_send_mac_learn_msg;
    mac_learn_digest_t meta_4_mac_learn_msg;
    state start {
        parsed_hdr_3_ethernet.setInvalid();
        parsed_hdr_3_ipv4.setInvalid();
        meta_4_send_mac_learn_msg = meta.send_mac_learn_msg;
        meta_4_mac_learn_msg.srcAddr = meta.mac_learn_msg.srcAddr;
        meta_4_mac_learn_msg.ingress_port = meta.mac_learn_msg.ingress_port;
        buffer.extract<ethernet_t>(parsed_hdr_3_ethernet);
        transition select(parsed_hdr_3_ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4_0;
            default: start_1;
        }
    }
    state CommonParser_parse_ipv4_0 {
        buffer.extract<ipv4_t>(parsed_hdr_3_ipv4);
        transition start_1;
    }
    state start_1 {
        parsed_hdr.ethernet = parsed_hdr_3_ethernet;
        parsed_hdr.ipv4 = parsed_hdr_3_ipv4;
        meta.send_mac_learn_msg = meta_4_send_mac_learn_msg;
        meta.mac_learn_msg.srcAddr = meta_4_mac_learn_msg.srcAddr;
        meta.mac_learn_msg.ingress_port = meta_4_mac_learn_msg.ingress_port;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.unknown_source") action unknown_source_0() {
        meta.send_mac_learn_msg = true;
        meta.mac_learn_msg.srcAddr = hdr.ethernet.srcAddr;
        meta.mac_learn_msg.ingress_port = istd.ingress_port;
    }
    @name("ingress.learned_sources") table learned_sources {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        actions = {
            NoAction_0();
            unknown_source_0();
        }
        default_action = unknown_source_0();
    }
    @name("ingress.do_L2_forward") action do_L2_forward_0(PortId_t egress_port) {
        ostd.drop = false;
        ostd.multicast_group = 10w0;
        ostd.egress_port = egress_port;
    }
    @name("ingress.l2_tbl") table l2_tbl {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        actions = {
            do_L2_forward_0();
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action act() {
        meta.send_mac_learn_msg = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        learned_sources.apply();
        l2_tbl.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    @name("IngressDeparserImpl.mac_learn_digest") Digest<mac_learn_digest_t>() mac_learn_digest;
    @hidden action act_0() {
        mac_learn_digest.pack(meta.mac_learn_msg);
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
        if (meta.send_mac_learn_msg) 
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

