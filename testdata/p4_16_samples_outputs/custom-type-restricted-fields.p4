#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

@p4runtime_translation("com.fingerhutpress/andysp4arch/v1/EthernetAddr_t" , 32) type bit<48> EthernetAddr_t;
@p4runtime_translation("com.fingerhutpress/andysp4arch/v1/IPv4Addr_t" , 32) type bit<32> IPv4Addr_t;
@p4runtime_translation("com.fingerhutpress/andysp4arch/v1/CustomAddr_t" , 32) type bit<32> CustomAddr_t;
header ethernet_t {
    EthernetAddr_t dstAddr;
    EthernetAddr_t srcAddr;
    bit<16>        etherType;
}

@controller_header("packet_out") header packet_out_t {
    bit<9>         egress_port;
    bit<8>         queue_id;
    EthernetAddr_t not_actually_useful;
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

header andycustom_t {
    bit<2>       version;
    bit<6>       dscp;
    bit<16>      totalLen;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<8>       l4Offset;
    bit<8>       flags;
    bit<8>       rsvd;
    CustomAddr_t srcAddr;
    CustomAddr_t dstAddr;
}

struct headers_t {
    packet_out_t cpu;
    ethernet_t   ethernet;
    ipv4_t       ipv4;
    andycustom_t andycustom;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    const bit<16> ETHERTYPE_IPV4 = 16w0x800;
    const bit<16> ETHERTYPE_ANDYCUSTOM = 16w0xd00d;
    const bit<8> IP_PROTOCOLS_IPV4 = 4;
    const bit<9> CPU_PORT = 111;
    state start {
        transition select(stdmeta.ingress_port) {
            CPU_PORT: parse_cpu_hdr;
            default: parse_ethernet;
        }
    }
    state parse_cpu_hdr {
        packet.extract(hdr.cpu);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_ANDYCUSTOM: parse_andycustom;
            default: accept;
        }
    }
    state parse_andycustom {
        packet.extract(hdr.andycustom);
        transition select(hdr.andycustom.protocol) {
            IP_PROTOCOLS_IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    action my_drop() {
        mark_to_drop(stdmeta);
    }
    action set_addr(IPv4Addr_t new_dstAddr) {
        hdr.ipv4.dstAddr = new_dstAddr;
        stdmeta.egress_spec = stdmeta.ingress_port;
    }
    table t1 {
        key = {
            hdr.andycustom.srcAddr: exact;
        }
        actions = {
            set_addr;
            my_drop;
            NoAction;
        }
        const default_action = NoAction;
    }
    apply {
        t1.apply();
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
