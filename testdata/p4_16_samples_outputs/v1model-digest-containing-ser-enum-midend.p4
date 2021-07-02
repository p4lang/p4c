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

struct test_digest_t {
    macAddr_t     in_mac_srcAddr;
    error         my_parser_error;
    MyPacketTypes pkt_type;
}

struct test_digest2_t {
    macAddr_t in_mac_dstAddr;
    bit<8>    my_thing;
}

struct test_digest3_t {
    bit<16> in_mac_etherType;
}

struct metadata {
    bit<48>       _test_digest_in_mac_srcAddr0;
    error         _test_digest_my_parser_error1;
    MyPacketTypes _test_digest_pkt_type2;
    bit<48>       _test_digest2_in_mac_dstAddr3;
    bit<8>        _test_digest2_my_thing4;
    bit<16>       _test_digest3_in_mac_etherType5;
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
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
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
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
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
        meta._test_digest_in_mac_srcAddr0 = hdr.ethernet.srcAddr;
        meta._test_digest_my_parser_error1 = error.PacketTooShort;
        meta._test_digest_pkt_type2 = MyPacketTypes.IPv4;
        digest<test_digest_t>(32w1, (test_digest_t){in_mac_srcAddr = hdr.ethernet.srcAddr,my_parser_error = error.PacketTooShort,pkt_type = MyPacketTypes.IPv4});
        meta._test_digest2_in_mac_dstAddr3 = hdr.ethernet.dstAddr;
        meta._test_digest2_my_thing4 = 8w42;
        digest<test_digest2_t>(32w2, (test_digest2_t){in_mac_dstAddr = hdr.ethernet.dstAddr,my_thing = 8w42});
        meta._test_digest3_in_mac_etherType5 = hdr.ethernet.etherType;
        digest<test_digest3_t>(32w3, (test_digest3_t){in_mac_etherType = hdr.ethernet.etherType});
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
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @name("MyEgress.rewrite_mac") action rewrite_mac(@name("srcAddr") macAddr_t srcAddr_1) {
        hdr.ethernet.srcAddr = srcAddr_1;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("MyEgress.send_frame") table send_frame_0 {
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
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

struct tuple_0 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<3>  f5;
    bit<13> f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<32> f9;
    bit<32> f10;
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

