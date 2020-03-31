#include <core.p4>
#include <v1model.p4>

typedef bit<9> egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;
header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

struct metadata {
    bool test_bool;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: reject;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarnUnused @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.drop") action drop() {
        mark_to_drop(standard_metadata);
    }
    @name("ingress.drop") action drop_2() {
        mark_to_drop(standard_metadata);
    }
    @name("ingress.ipv4_forward") action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {
        meta.test_bool = true;
    }
    @name("ingress.ipv4_lpm") table ipv4_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            NoAction_0();
            ipv4_forward();
            drop();
        }
        size = 1024;
        default_action = NoAction_0();
    }
    @hidden action flag_lostbmv2l86() {
        meta.test_bool = false;
    }
    @hidden table tbl_flag_lostbmv2l86 {
        actions = {
            flag_lostbmv2l86();
        }
        const default_action = flag_lostbmv2l86();
    }
    @hidden table tbl_drop {
        actions = {
            drop_2();
        }
        const default_action = drop_2();
    }
    apply {
        tbl_flag_lostbmv2l86.apply();
        if (hdr.ipv4.isValid()) {
            ipv4_lpm_0.apply();
        }
        if (!meta.test_bool) {
            tbl_drop.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

