#include <core.p4>
#include <pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
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

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct main_metadata_t {
    bit<32> dst_port;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.key_0") bit<32> key_0;
    @name("MainControlImpl.key_1") bit<32> key_1;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.clb_pinned_flows") table clb_pinned_flows_0 {
        key = {
            key_0            : exact @name("ipv4_addr_0");
            key_1            : exact @name("ipv4_addr_1");
            hdr.ipv4.protocol: exact @name("hdr.ipv4.protocol");
        }
        actions = {
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    @hidden action issue2900l63() {
        key_0 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr);
        key_1 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr);
    }
    @hidden action issue2900l63_0() {
        key_0 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr);
        key_1 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr);
    }
    @hidden table tbl_issue2900l63 {
        actions = {
            issue2900l63();
        }
        const default_action = issue2900l63();
    }
    @hidden table tbl_issue2900l63_0 {
        actions = {
            issue2900l63_0();
        }
        const default_action = issue2900l63_0();
    }
    apply {
        if (istd.direction == PNA_Direction_t.HOST_TO_NET) {
            tbl_issue2900l63.apply();
            clb_pinned_flows_0.apply();
        } else {
            tbl_issue2900l63_0.apply();
            clb_pinned_flows_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action issue2900l83() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_issue2900l83 {
        actions = {
            issue2900l83();
        }
        const default_action = issue2900l83();
    }
    apply {
        tbl_issue2900l83.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
