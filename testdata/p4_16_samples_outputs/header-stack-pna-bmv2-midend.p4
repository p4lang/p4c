#include <core.p4>

#include <bmv2/pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header vlan_t {
    bit<16> tci;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
    vlan_t[2]  vlan;
}

struct metadata_t {
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x8100: parse_vlan;
            default: accept;
        }
    }
    state parse_vlan {
        pkt.extract<vlan_t>(hdr.vlan.next);
        transition select(hdr.vlan.last.etherType) {
            16w0x8100: parse_vlan;
            default: accept;
        }
    }
}

control MainControlImpl(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action headerstackpnabmv2l51() {
        hdr.vlan.pop_front(1);
        hdr.ethernet.etherType = 16w0x8100;
    }
    @hidden action headerstackpnabmv2l54() {
        send_to_port(32w1);
    }
    @hidden table tbl_headerstackpnabmv2l51 {
        actions = {
            headerstackpnabmv2l51();
        }
        const default_action = headerstackpnabmv2l51();
    }
    @hidden table tbl_headerstackpnabmv2l54 {
        actions = {
            headerstackpnabmv2l54();
        }
        const default_action = headerstackpnabmv2l54();
    }
    apply {
        if (hdr.vlan[1].isValid()) {
            tbl_headerstackpnabmv2l51.apply();
        }
        tbl_headerstackpnabmv2l54.apply();
    }
}

control MainDeparserImpl(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action headerstackpnabmv2l61() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<vlan_t>(hdr.vlan[0]);
        pkt.emit<vlan_t>(hdr.vlan[1]);
    }
    @hidden table tbl_headerstackpnabmv2l61 {
        actions = {
            headerstackpnabmv2l61();
        }
        const default_action = headerstackpnabmv2l61();
    }
    apply {
        tbl_headerstackpnabmv2l61.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(MainParserImpl(), MainControlImpl(), MainDeparserImpl()) main;
