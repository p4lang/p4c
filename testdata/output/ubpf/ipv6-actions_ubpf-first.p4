#include <core.p4>
#include <ubpf_model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header IPv4_h {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

header IPv6_h {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

header mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit<1>  stack;
    bit<8>  ttl;
}

struct Headers_t {
    Ethernet_h ethernet;
    mpls_h     mpls;
    IPv4_h     ipv4;
    IPv6_h     ipv6;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ipv4;
            16w0x86dd: ipv6;
            16w0x8847: mpls;
            default: reject;
        }
    }
    state mpls {
        p.extract<mpls_h>(headers.mpls);
        transition ipv4;
    }
    state ipv4 {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
    state ipv6 {
        p.extract<IPv6_h>(headers.ipv6);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
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
    action set_traffic_class(bit<8> trafficClass) {
        headers.ipv6.trafficClass = trafficClass;
    }
    action set_traffic_class_flow_label(bit<8> trafficClass, bit<20> label) {
        headers.ipv6.trafficClass = trafficClass;
        headers.ipv6.flowLabel = label;
    }
    action set_ipv6_version(bit<4> version) {
        headers.ipv6.version = version;
    }
    action set_next_hdr(bit<8> nextHdr) {
        headers.ipv6.nextHdr = nextHdr;
    }
    action set_hop_limit(bit<8> hopLimit) {
        headers.ipv6.hopLimit = hopLimit;
    }
    table filter_tbl {
        key = {
            headers.ipv6.srcAddr: exact @name("headers.ipv6.srcAddr");
        }
        actions = {
            ipv6_modify_dstAddr();
            ipv6_swap_addr();
            set_flowlabel();
            set_traffic_class_flow_label();
            set_ipv6_version();
            set_next_hdr();
            set_hop_limit();
            Reject();
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        filter_tbl.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<mpls_h>(headers.mpls);
        packet.emit<IPv6_h>(headers.ipv6);
        packet.emit<IPv4_h>(headers.ipv4);
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
