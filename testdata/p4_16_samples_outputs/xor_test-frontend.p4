#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    bit<32> before1;
    bit<32> before2;
    bit<32> before3;
    bit<32> before4;
    bit<32> after1;
    bit<32> after2;
    bit<32> after3;
    bit<32> after4;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("MyIngress.forward_and_do_something") action forward_and_do_something(@name("port") egressSpec_t port) {
        standard_metadata.egress_spec = port;
        if (hdr.ipv4.isValid()) {
            meta.before1 = hdr.ipv4.srcAddr;
            hdr.ipv4.srcAddr = hdr.ipv4.srcAddr ^ 32w0x12345678;
            meta.after1 = hdr.ipv4.srcAddr;
        }
        if (hdr.ethernet.isValid()) {
            if (hdr.ethernet.isValid()) {
                hdr.ipv4.protocol = hdr.ipv4.protocol ^ 8w1;
            }
            meta.before2 = hdr.ipv4.dstAddr;
            hdr.ipv4.dstAddr = hdr.ipv4.dstAddr ^ 32w0x12345678;
            meta.after2 = hdr.ipv4.dstAddr;
        }
        meta.before3 = hdr.ipv4.srcAddr;
        hdr.ipv4.srcAddr = hdr.ipv4.srcAddr ^ 32w0x12345678;
        meta.after3 = hdr.ipv4.srcAddr;
        meta.before4 = hdr.ipv4.dstAddr;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr ^ 32w0x12345678;
        meta.after4 = hdr.ipv4.dstAddr;
    }
    @name("MyIngress.ipv4_lpm") table ipv4_lpm_0 {
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port");
        }
        actions = {
            forward_and_do_something();
            NoAction_1();
        }
        const entries = {
                        9w1 : forward_and_do_something(9w2);
                        9w2 : forward_and_do_something(9w1);
        }
        default_action = NoAction_1();
    }
    @name("MyIngress.debug") table debug_0 {
        key = {
            meta.before1: exact @name("meta.before1");
            meta.after1 : exact @name("meta.after1");
            meta.before2: exact @name("meta.before2");
            meta.after2 : exact @name("meta.after2");
            meta.before3: exact @name("meta.before3");
            meta.after3 : exact @name("meta.after3");
            meta.before4: exact @name("meta.before4");
            meta.after4 : exact @name("meta.after4");
        }
        actions = {
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_lpm_0.apply();
            debug_0.apply();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
