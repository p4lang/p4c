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
    headers parsed_hdr_0;
    metadata meta_1;
    state start {
        parsed_hdr_0.ethernet.setInvalid();
        parsed_hdr_0.ipv4.setInvalid();
        meta_1 = meta;
        transition CommonParser_start;
    }
    state CommonParser_start {
        buffer.extract<ethernet_t>(parsed_hdr_0.ethernet);
        transition select(parsed_hdr_0.ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4;
            default: start_0;
        }
    }
    state CommonParser_parse_ipv4 {
        buffer.extract<ipv4_t>(parsed_hdr_0.ipv4);
        transition start_0;
    }
    state start_0 {
        parsed_hdr = parsed_hdr_0;
        meta = meta_1;
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    headers parsed_hdr_1;
    metadata meta_2;
    state start {
        parsed_hdr_1.ethernet.setInvalid();
        parsed_hdr_1.ipv4.setInvalid();
        meta_2 = meta;
        transition CommonParser_start_0;
    }
    state CommonParser_start_0 {
        buffer.extract<ethernet_t>(parsed_hdr_1.ethernet);
        transition select(parsed_hdr_1.ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4_0;
            default: start_1;
        }
    }
    state CommonParser_parse_ipv4_0 {
        buffer.extract<ipv4_t>(parsed_hdr_1.ipv4);
        transition start_1;
    }
    state start_1 {
        parsed_hdr = parsed_hdr_1;
        meta = meta_2;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.unknown_source") action unknown_source() {
        meta.send_mac_learn_msg = true;
        meta.mac_learn_msg.srcAddr = hdr.ethernet.srcAddr;
        meta.mac_learn_msg.ingress_port = istd.ingress_port;
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
        {
            psa_ingress_output_metadata_t meta_3 = ostd;
            PortId_t egress_port_1 = egress_port;
            meta_3.drop = false;
            meta_3.multicast_group = (MulticastGroup_t)32w0;
            meta_3.egress_port = egress_port_1;
            ostd = meta_3;
        }
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
    apply {
        meta.send_mac_learn_msg = false;
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
    apply {
        if (meta.send_mac_learn_msg) 
            mac_learn_digest_0.pack(meta.mac_learn_msg);
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

