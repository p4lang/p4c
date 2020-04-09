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
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ipv4;
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
}

control pipe(inout Headers_t headers, inout metadata meta) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    bit<32> tmp_0;
    @name("pipe.ip_modify_saddr") action ip_modify_saddr(bit<32> srcAddr) {
        headers.ipv4.srcAddr = srcAddr;
    }
    @name("pipe.mpls_modify_tc") action mpls_modify_tc(bit<3> tc) {
        headers.mpls.tc = tc;
    }
    @name("pipe.mpls_set_label") action mpls_set_label(bit<20> label) {
        headers.mpls.label = label;
    }
    @name("pipe.mpls_set_label_tc") action mpls_set_label_tc(bit<20> label, bit<3> tc) {
        headers.mpls.label = label;
        headers.mpls.tc = tc;
    }
    @name("pipe.mpls_decrement_ttl") action mpls_decrement_ttl() {
        headers.mpls.ttl = headers.mpls.ttl + 8w255;
    }
    @name("pipe.mpls_set_label_decrement_ttl") action mpls_set_label_decrement_ttl(bit<20> label) {
        headers.mpls.label = label;
        headers.mpls.ttl = headers.mpls.ttl + 8w255;
    }
    @name("pipe.mpls_modify_stack") action mpls_modify_stack(bit<1> stack) {
        headers.mpls.stack = stack;
    }
    @name("pipe.change_ip_ver") action change_ip_ver() {
        headers.ipv4.version = 4w6;
    }
    @name("pipe.ip_swap_addrs") action ip_swap_addrs() {
        tmp_0 = headers.ipv4.dstAddr;
        headers.ipv4.dstAddr = headers.ipv4.srcAddr;
        headers.ipv4.srcAddr = tmp_0;
    }
    @name("pipe.Reject") action Reject() {
        mark_to_drop();
    }
    @name("pipe.filter_tbl") table filter_tbl_0 {
        key = {
            headers.ipv4.dstAddr: exact @name("headers.ipv4.dstAddr") ;
        }
        actions = {
            mpls_decrement_ttl();
            mpls_set_label();
            mpls_set_label_decrement_ttl();
            mpls_modify_tc();
            mpls_set_label_tc();
            mpls_modify_stack();
            change_ip_ver();
            ip_swap_addrs();
            ip_modify_saddr();
            Reject();
            NoAction_0();
        }
        const default_action = NoAction_0();
    }
    apply {
        filter_tbl_0.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<mpls_h>(headers.mpls);
        packet.emit<IPv4_h>(headers.ipv4);
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

