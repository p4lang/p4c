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

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
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

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.tmp") bit<32> tmp_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("pipe.ip_modify_saddr") action ip_modify_saddr(@name("srcAddr") bit<32> srcAddr_1) {
        headers.ipv4.srcAddr = srcAddr_1;
    }
    @name("pipe.mpls_modify_tc") action mpls_modify_tc(@name("tc") bit<3> tc_2) {
        headers.mpls.tc = tc_2;
    }
    @name("pipe.mpls_set_label") action mpls_set_label(@name("label") bit<20> label_3) {
        headers.mpls.label = label_3;
    }
    @name("pipe.mpls_set_label_tc") action mpls_set_label_tc(@name("label") bit<20> label_4, @name("tc") bit<3> tc_3) {
        headers.mpls.label = label_4;
        headers.mpls.tc = tc_3;
    }
    @name("pipe.mpls_decrement_ttl") action mpls_decrement_ttl() {
        headers.mpls.ttl = headers.mpls.ttl + 8w255;
    }
    @name("pipe.mpls_set_label_decrement_ttl") action mpls_set_label_decrement_ttl(@name("label") bit<20> label_5) {
        headers.mpls.label = label_5;
        headers.mpls.ttl = headers.mpls.ttl + 8w255;
    }
    @name("pipe.mpls_modify_stack") action mpls_modify_stack(@name("stack") bit<1> stack_1) {
        headers.mpls.stack = stack_1;
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
            headers.ipv4.dstAddr: exact @name("headers.ipv4.dstAddr");
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
            NoAction_1();
        }
        default_action = NoAction_1();
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
