#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<9> egressSpec_t;
typedef bit<32> foo_t;
typedef foo_t bar_t;
typedef bit<48> EthernetAddr_t;
typedef bit<32> IPv4Addr_t;
header ethernet_t {
    EthernetAddr_t dstAddr;
    EthernetAddr_t srcAddr;
    bit<16>        etherType;
}

header ipv4_t {
    bit<4>     version;
    bit<4>     ihl;
    bit<8>     diffserv;
    bit<16>    totalLen;
    bit<16>    identification;
    bit<3>     flags;
    bit<13>    fragOffset;
    bit<8>     ttl;
    bit<8>     protocol;
    bit<16>    hdrChecksum;
    IPv4Addr_t srcAddr;
    IPv4Addr_t dstAddr;
}

enum MyPacketTypes {
    IPv4,
    Other
}

struct test_digest_t {
    EthernetAddr_t in_mac_srcAddr;
    error          my_parser_error;
    MyPacketTypes  pkt_type;
    bar_t          bar;
}

struct metadata {
    bit<48>       _test_digest_in_mac_srcAddr0;
    error         _test_digest_my_parser_error1;
    MyPacketTypes _test_digest_pkt_type2;
    bit<32>       _test_digest_bar3;
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
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngress.set_dmac") action set_dmac(EthernetAddr_t dstAddr) {
        hdr.ethernet.dstAddr = dstAddr;
    }
    @name("MyIngress.drop") action drop() {
    }
    @name("MyIngress.drop") action drop_2() {
    }
    @name("MyIngress.forward") table forward_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            set_dmac();
            drop();
            NoAction_0();
        }
        size = 1024;
        default_action = NoAction_0();
    }
    @name("MyIngress.set_nhop") action set_nhop(IPv4Addr_t dstAddr, egressSpec_t port) {
        hdr.ipv4.dstAddr = dstAddr;
        standard_metadata.egress_spec = port;
    }
    @name("MyIngress.ipv4_lpm") table ipv4_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            set_nhop();
            drop_2();
            NoAction_1();
        }
        size = 1024;
        default_action = NoAction_1();
    }
    @name("MyIngress.send_digest") action send_digest() {
        meta._test_digest_in_mac_srcAddr0 = hdr.ethernet.srcAddr;
        meta._test_digest_my_parser_error1 = error.PacketTooShort;
        meta._test_digest_pkt_type2 = MyPacketTypes.IPv4;
        meta._test_digest_bar3 = 32w777;
        digest<test_digest_t>(32w1, (test_digest_t){in_mac_srcAddr = hdr.ethernet.srcAddr,my_parser_error = error.PacketTooShort,pkt_type = MyPacketTypes.IPv4,bar = 32w777});
    }
    @hidden table tbl_send_digest {
        actions = {
            send_digest();
        }
        const default_action = send_digest();
    }
    apply {
        ipv4_lpm_0.apply();
        forward_0.apply();
        tbl_send_digest.apply();
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @name("MyEgress.rewrite_mac") action rewrite_mac(EthernetAddr_t srcAddr) {
        hdr.ethernet.srcAddr = srcAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("MyEgress.send_frame") table send_frame_0 {
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
        }
        actions = {
            rewrite_mac();
            NoAction_5();
        }
        size = 1024;
        default_action = NoAction_5();
    }
    apply {
        send_frame_0.apply();
    }
}

struct tuple_0 {
    bit<4>  field;
    bit<4>  field_0;
    bit<8>  field_1;
    bit<16> field_2;
    bit<16> field_3;
    bit<3>  field_4;
    bit<13> field_5;
    bit<8>  field_6;
    bit<8>  field_7;
    bit<32> field_8;
    bit<32> field_9;
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

