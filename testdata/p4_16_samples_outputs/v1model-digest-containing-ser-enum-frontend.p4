error {
    UnreachableState
}
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

enum MyPacketTypes {
    IPv4,
    Other
}

enum bit<8> MySerEnum1 {
    foo = 8w28,
    bar = 8w17,
    gah = 8w42
}

struct test_digest_t {
    macAddr_t     in_mac_srcAddr;
    error         my_parser_error;
    MyPacketTypes pkt_type;
}

struct test_digest2_t {
    macAddr_t  in_mac_dstAddr;
    MySerEnum1 my_thing;
}

struct test_digest3_t {
    bit<16> in_mac_etherType;
}

struct metadata {
    test_digest_t  test_digest;
    test_digest2_t test_digest2;
    test_digest3_t test_digest3;
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
            default: accept;
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
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("MyIngress.set_dmac") action set_dmac(@name("dstAddr") macAddr_t dstAddr_2) {
        hdr.ethernet.dstAddr = dstAddr_2;
    }
    @name("MyIngress.drop") action drop() {
    }
    @name("MyIngress.drop") action drop_1() {
    }
    @name("MyIngress.forward") table forward_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr");
        }
        actions = {
            set_dmac();
            drop();
            NoAction_2();
        }
        size = 1024;
        default_action = NoAction_2();
    }
    @name("MyIngress.set_nhop") action set_nhop(@name("dstAddr") ip4Addr_t dstAddr_3, @name("port") egressSpec_t port) {
        hdr.ipv4.dstAddr = dstAddr_3;
        standard_metadata.egress_spec = port;
    }
    @name("MyIngress.ipv4_lpm") table ipv4_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        actions = {
            set_nhop();
            drop_1();
            NoAction_3();
        }
        size = 1024;
        default_action = NoAction_3();
    }
    @name("MyIngress.send_digest") action send_digest() {
        meta.test_digest.in_mac_srcAddr = hdr.ethernet.srcAddr;
        meta.test_digest.my_parser_error = error.PacketTooShort;
        meta.test_digest.pkt_type = MyPacketTypes.IPv4;
        digest<test_digest_t>(32w1, meta.test_digest);
        meta.test_digest2.in_mac_dstAddr = hdr.ethernet.dstAddr;
        meta.test_digest2.my_thing = MySerEnum1.gah;
        digest<test_digest2_t>(32w2, meta.test_digest2);
        meta.test_digest3.in_mac_etherType = hdr.ethernet.etherType;
        digest<test_digest3_t>(32w3, meta.test_digest3);
    }
    apply {
        ipv4_lpm_0.apply();
        forward_0.apply();
        send_digest();
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @name("MyEgress.rewrite_mac") action rewrite_mac(@name("srcAddr") macAddr_t srcAddr_1) {
        hdr.ethernet.srcAddr = srcAddr_1;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("MyEgress.send_frame") table send_frame_0 {
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port");
        }
        actions = {
            rewrite_mac();
            NoAction_4();
        }
        size = 1024;
        default_action = NoAction_4();
    }
    apply {
        send_frame_0.apply();
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
