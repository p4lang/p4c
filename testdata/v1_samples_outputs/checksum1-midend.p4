#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

struct metadata {
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
    @name("ipv6") 
    ipv6_t     ipv6;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            default: accept;
        }
    }
    @name("parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
    @name("parse_ipv6") state parse_ipv6 {
        packet.extract(hdr.ipv6);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("drop") action drop_0() {
    }
    @name("forward") action forward_0(bit<48> to) {
        hdr.ethernet.dstAddr = to;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("do_setup") action do_setup_0() {
    }
    @name("route") table route_0() {
        actions = {
            drop_0;
            forward_0;
            NoAction_0;
        }
        key = {
            hdr.ipv4.dstAddr: ternary;
        }
        default_action = NoAction_0();
    }
    @name("setup") table setup_0() {
        actions = {
            do_setup_0;
            NoAction_0;
        }
        key = {
            hdr.ethernet.isValid(): exact;
        }
        default_action = NoAction_0();
    }
    apply {
        setup_0.apply();
        route_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv6);
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
