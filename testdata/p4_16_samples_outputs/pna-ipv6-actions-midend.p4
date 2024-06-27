#include <core.p4>
#include <pna.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header IPv4_h {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
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

struct headers_t {
    Ethernet_h ethernet;
    mpls_h     mpls;
    IPv4_h     ipv4;
    IPv6_h     ipv6;
}

struct main_metadata_t {
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in p, out headers_t headers, inout main_metadata_t meta, in pna_main_parser_input_metadata_t istd) {
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

control MainControlImpl(inout headers_t headers, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.tmp") bit<128> tmp_0;
    @name("MainControlImpl.tmp1") bit<32> tmp1_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.Reject") action Reject() {
        drop_packet();
    }
    @name("MainControlImpl.ipv6_modify_dstAddr") action ipv6_modify_dstAddr(@name("dstAddr") bit<32> dstAddr_1) {
        headers.ipv6.dstAddr = (bit<128>)dstAddr_1;
    }
    @name("MainControlImpl.ipv6_swap_addr") action ipv6_swap_addr() {
        headers.ipv6.dstAddr = headers.ipv6.srcAddr;
        headers.ipv6.srcAddr = tmp_0;
    }
    @name("MainControlImpl.set_flowlabel") action set_flowlabel(@name("label") bit<20> label_2) {
        headers.ipv6.flowLabel = label_2;
    }
    @name("MainControlImpl.set_traffic_class_flow_label") action set_traffic_class_flow_label(@name("trafficClass") bit<8> trafficClass_1, @name("label") bit<20> label_3) {
        headers.ipv6.trafficClass = trafficClass_1;
        headers.ipv6.flowLabel = label_3;
        headers.ipv6.srcAddr = (bit<128>)tmp1_0;
    }
    @name("MainControlImpl.set_ipv6_version") action set_ipv6_version(@name("version") bit<4> version_1) {
        headers.ipv6.version = version_1;
    }
    @name("MainControlImpl.set_next_hdr") action set_next_hdr(@name("nextHdr") bit<8> nextHdr_1) {
        headers.ipv6.nextHdr = nextHdr_1;
    }
    @name("MainControlImpl.set_hop_limit") action set_hop_limit(@name("hopLimit") bit<8> hopLimit_1) {
        headers.ipv6.hopLimit = hopLimit_1;
    }
    @name("MainControlImpl.filter_tbl") table filter_tbl_0 {
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
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action pnaipv6actions109() {
        tmp_0 = 128w0x76;
    }
    @hidden table tbl_pnaipv6actions109 {
        actions = {
            pnaipv6actions109();
        }
        const default_action = pnaipv6actions109();
    }
    apply {
        tbl_pnaipv6actions109.apply();
        filter_tbl_0.apply();
    }
}

control MainDeparserImpl(packet_out packet, in headers_t headers, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaipv6actions180() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<mpls_h>(headers.mpls);
        packet.emit<IPv6_h>(headers.ipv6);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_pnaipv6actions180 {
        actions = {
            pnaipv6actions180();
        }
        const default_action = pnaipv6actions180();
    }
    apply {
        tbl_pnaipv6actions180.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
