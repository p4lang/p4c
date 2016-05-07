#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

struct routing_metadata_t {
    bit<32> nhop_ipv4;
}

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

struct metadata {
    @name("routing_metadata") 
    routing_metadata_t routing_metadata;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name("parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("rewrite_mac") action rewrite_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("_drop") action _drop_0() {
        mark_to_drop();
    }
    @name("send_frame") table send_frame_0() {
        actions = {
            rewrite_mac_0;
            _drop_0;
            NoAction_0;
        }
        key = {
            standard_metadata.egress_port: exact;
        }
        size = 256;
        default_action = NoAction_0();
    }
    apply {
        send_frame_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_1() {
    }
    action NoAction_2() {
    }
    @name("set_dmac") action set_dmac_0(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("_drop") action _drop_1() {
        mark_to_drop();
    }
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("set_nhop") action set_nhop_0(bit<32> nhop_ipv4, bit<9> port) {
        meta.routing_metadata.nhop_ipv4 = nhop_ipv4;
        standard_metadata.egress_port = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("forward") table forward_0() {
        actions = {
            set_dmac_0;
            _drop_1;
            NoAction_1;
        }
        key = {
            meta.routing_metadata.nhop_ipv4: exact;
        }
        size = 512;
        default_action = NoAction_1();
    }
    @name("ipv4_lpm") table ipv4_lpm_0() {
        actions = {
            set_nhop_0;
            _drop;
            NoAction_2;
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 1024;
        default_action = NoAction_1();
    }
    apply {
        if (hdr.ipv4.isValid() && hdr.ipv4.ttl > 8w0) {
            ipv4_lpm_0.apply();
            forward_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    Checksum16() @name("ipv4_checksum") ipv4_checksum_0;
    action act() {
        standard_metadata.drop = 1w1;
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    apply {
        if (hdr.ipv4.hdrChecksum == ipv4_checksum_0.get({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr })) 
            tbl_act.apply();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    Checksum16() @name("ipv4_checksum") ipv4_checksum_1;
    action act_0() {
        hdr.ipv4.hdrChecksum = ipv4_checksum_1.get({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
    }
    table tbl_act_0() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    apply {
        tbl_act_0.apply();
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
