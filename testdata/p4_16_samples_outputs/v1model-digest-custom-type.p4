#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

const bit<16> TYPE_IPV4 = 0x800;
typedef bit<9> egressSpec_t;
typedef bit<32> switchID_t;
typedef bit<19> qdepth_t;
typedef bit<48> timestamp_t;
typedef bit<32> timedelta_t;
type bit<32> foo_t;
type foo_t bar_t;
@p4runtime_translation("com.fingerhutpress/andysp4arch/v1/EthernetAddr_t" , 32) type bit<48> EthernetAddr_t;
@p4runtime_translation("com.fingerhutpress/andysp4arch/v1/IPv4Addr_t" , 32) type bit<32> IPv4Addr_t;
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
    test_digest_t test_digest;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action set_dmac(EthernetAddr_t dstAddr) {
        hdr.ethernet.dstAddr = dstAddr;
    }
    action drop() {
    }
    table forward {
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        actions = {
            set_dmac;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }
    action set_nhop(IPv4Addr_t dstAddr, egressSpec_t port) {
        hdr.ipv4.dstAddr = dstAddr;
        standard_metadata.egress_spec = port;
    }
    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            set_nhop;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }
    action send_digest() {
        meta.test_digest.in_mac_srcAddr = hdr.ethernet.srcAddr;
        meta.test_digest.my_parser_error = error.PacketTooShort;
        meta.test_digest.pkt_type = MyPacketTypes.IPv4;
        meta.test_digest.bar = (bar_t)(foo_t)32w777;
        digest(1, meta.test_digest);
    }
    apply {
        ipv4_lpm.apply();
        forward.apply();
        send_digest();
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action rewrite_mac(EthernetAddr_t srcAddr) {
        hdr.ethernet.srcAddr = srcAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }
    table send_frame {
        key = {
            standard_metadata.egress_port: exact;
        }
        actions = {
            rewrite_mac;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        send_frame.apply();
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

