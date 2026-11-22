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
    bit<32>         tmpDir;
    PNA_Direction_t t1;
    bool            b;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    @hidden action pnadirection73() {
        meta.tmpDir = hdr.ipv4.srcAddr;
    }
    @hidden action pnadirection75() {
        meta.tmpDir = hdr.ipv4.dstAddr;
    }
    @hidden action pnadirection71() {
        meta.b = istd.direction != PNA_Direction_t.NET_TO_HOST;
    }
    @hidden table tbl_pnadirection71 {
        actions = {
            pnadirection71();
        }
        const default_action = pnadirection71();
    }
    @hidden table tbl_pnadirection73 {
        actions = {
            pnadirection73();
        }
        const default_action = pnadirection73();
    }
    @hidden table tbl_pnadirection75 {
        actions = {
            pnadirection75();
        }
        const default_action = pnadirection75();
    }
    apply {
        tbl_pnadirection71.apply();
        if (PNA_Direction_t.NET_TO_HOST == istd.direction) {
            tbl_pnadirection73.apply();
        } else {
            tbl_pnadirection75.apply();
        }
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
    @name("MainControlImpl.tmpDir") bit<32> tmpDir_0;
    @name("MainControlImpl.next_hop") action next_hop(@name("vport") bit<32> vport) {
        send_to_port(vport);
    }
    @name("MainControlImpl.default_route_drop") action default_route_drop() {
        drop_packet();
    }
    @name("MainControlImpl.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            tmpDir_0: lpm @name("ipv4_addr");
        }
        actions = {
            next_hop();
            default_route_drop();
        }
        const default_action = default_route_drop();
    }
    @hidden action pnadirection127() {
        tmpDir_0 = hdr.ipv4.srcAddr;
    }
    @hidden action pnadirection129() {
        tmpDir_0 = hdr.ipv4.dstAddr;
    }
    @hidden table tbl_pnadirection127 {
        actions = {
            pnadirection127();
        }
        const default_action = pnadirection127();
    }
    @hidden table tbl_pnadirection129 {
        actions = {
            pnadirection129();
        }
        const default_action = pnadirection129();
    }
    apply {
        if (PNA_Direction_t.NET_TO_HOST == istd.direction) {
            tbl_pnadirection127.apply();
        } else {
            tbl_pnadirection129.apply();
        }
        if (hdr.ipv4.isValid()) {
            ipv4_da_lpm_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadirection144() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnadirection144 {
        actions = {
            pnadirection144();
        }
        const default_action = pnadirection144();
    }
    apply {
        tbl_pnadirection144.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
