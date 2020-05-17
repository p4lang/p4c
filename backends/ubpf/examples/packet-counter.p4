#define UBPF_MODEL_VERSION 20200304
#include <ubpf_model.p4>
#include <core.p4>

typedef bit<48> EthernetAddress;
typedef bit<32>     IPv4Address;

#define IPV4_ETHTYPE 0x800
#define ETH_HDR_SIZE 14
#define IPV4_HDR_SIZE 20
#define UDP_HDR_SIZE 8
#define IP_VERSION_4 4
#define IPV4_MIN_IHL 5

const bit<8> PROTO_ICMP = 1;

// standard Ethernet header
header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

// IPv4 header without options
header IPv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    IPv4Address  srcAddr;
    IPv4Address  dstAddr;
}

header icmp_t {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
    bit<32> rest;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
    icmp_t     icmp;
}

struct metadata {}

parser prs(packet_in packet, out Headers_t hdr, inout metadata meta) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            IPV4_ETHTYPE: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            PROTO_ICMP: parce_icmp;
            default: accept;
        }
    }

    state parce_icmp {
        packet.extract(hdr.icmp);
        transition accept;
    }
}

control pipe(inout Headers_t hdr, inout metadata meta) {

    Register<bit<32>, bit<32>>(1) packet_counter_reg;

    apply {
        bit<32> last_count;
        bit<32> index = 0;
        last_count = packet_counter_reg.read(index);
        packet_counter_reg.write(index, last_count + 1);
    }
}

control dprs(packet_out packet, in Headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.icmp);
    }
}

ubpf(prs(), pipe(), dprs()) main;