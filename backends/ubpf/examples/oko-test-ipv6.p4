#include "ubpf_filter_model.p4"
#include <core.p4>

@ethernetaddress typedef bit<48> EthernetAddress;
@ipv4address     typedef bit<32>     IPv4Address;

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

header IPv6_h {
    bit<4>       version;
    bit<8>       trafficClass;
    bit<20>      flowLabel;
    bit<16>      payloadLen;
    bit<8>       nextHdr;
    bit<8>       hopLimit;
    bit<128>     srcAddr;
    bit<128>     dstAddr;
}

header mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit<1>  stack;
    bit<8>  ttl;
}

struct Headers_t
{
    Ethernet_h ethernet;
    mpls_h     mpls;
    IPv4_h     ipv4;
    IPv6_h     ipv6;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800 : ipv4;
            0x86DD   : ipv6;
            0x8847   : mpls;
            default : reject;
        }
    }

    state mpls {
            p.extract(headers.mpls);
            transition ipv4;
    }

    state ipv4 {
        p.extract(headers.ipv4);
        transition accept;
    }

    state ipv6 {
        p.extract(headers.ipv6);
        transition accept;
    }

}

control pipe(inout Headers_t headers) {

    action Reject() {
        mark_to_drop();
    }

    action ipv6_modify_dstAddr(bit<128> dstAddr) {
        headers.ipv6.dstAddr = dstAddr;
    }

    action ipv6_swap_addr() {
        bit<128> tmp = headers.ipv6.dstAddr;
        headers.ipv6.dstAddr = headers.ipv6.srcAddr;
        headers.ipv6.srcAddr = tmp;
    }

    action set_flowlabel(bit<20> label) {
        headers.ipv6.flowLabel = label;
    }

    table filter_tbl {
        key = {
            headers.ipv6.srcAddr : exact;
        }
        actions = {
            ipv6_modify_dstAddr;
            ipv6_swap_addr;
            set_flowlabel;
            Reject;
            NoAction;
        }

        const default_action = NoAction;
    }

    apply {
        filter_tbl.apply();
    }
}

ubpfFilter(prs(), pipe()) main;
