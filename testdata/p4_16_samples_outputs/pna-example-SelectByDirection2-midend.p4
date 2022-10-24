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

struct empty_metadata_t {
}

struct main_metadata_t {
    bit<32> meta;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
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

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.addr") bit<32> addr_0;
    @name("MainControlImpl.forward") action forward(@name("addr") bit<32> addr) {
        user_meta.meta = addr;
    }
    @name("MainControlImpl.forward") action forward_1() {
        addr_0 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr);
        user_meta.meta = addr_0;
    }
    @name("MainControlImpl.default_route_drop") action default_route_drop() {
        drop_packet();
    }
    @name("MainControlImpl.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        actions = {
            forward();
            default_route_drop();
        }
        const default_action = default_route_drop();
    }
    @hidden table tbl_forward {
        actions = {
            forward_1();
        }
        const default_action = forward_1();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            tbl_forward.apply();
            if (user_meta.meta == hdr.ipv4.dstAddr) {
                ipv4_da_lpm_0.apply();
            }
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexampleSelectByDirection2l138() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnaexampleSelectByDirection2l138 {
        actions = {
            pnaexampleSelectByDirection2l138();
        }
        const default_action = pnaexampleSelectByDirection2l138();
    }
    apply {
        tbl_pnaexampleSelectByDirection2l138.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
