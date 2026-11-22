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
    apply {
        if (hdr.vlan[1].isValid()) {
            hdr.vlan.pop_front(1);
            hdr.ethernet.etherType = 16w0x8100;
        }
        send_to_port((PortId_t)32w1);
    }
}

control MainDeparserImpl(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<vlan_t[2]>(hdr.vlan);
    }
}

PNA_NIC<headers_t, metadata_t>(MainParserImpl(), MainControlImpl(), MainDeparserImpl()) main;
