#include <core.p4>
#include <dpdk/pna.p4>

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
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    ethernet_t ethernet1;
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
    @name("MainControlImpl.tmpDir") bit<32> tmpDir_0;
    @name("MainControlImpl.eth") ethernet_t eth_0;
    @name("MainControlImpl.hdr_0") ethernet_t hdr_1;
    @name("MainControlImpl.outer_hdr_0") ethernet_t outer_hdr;
    @name("MainControlImpl.next_hop") action next_hop(@name("vport") bit<32> vport) {
        eth_0 = hdr.ethernet;
        hdr.ethernet1 = hdr.ethernet;
        send_to_port(vport);
    }
    @name("MainControlImpl.default_route_drop") action default_route_drop() {
        drop_packet();
        hdr_1 = hdr.ethernet;
        outer_hdr = hdr.ethernet1;
        hdr_1.dstAddr = outer_hdr.dstAddr;
        hdr_1.srcAddr = outer_hdr.srcAddr;
        hdr_1.etherType = outer_hdr.etherType;
        hdr.ethernet = hdr_1;
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
    @hidden action pnaelimhdrcopydpdk139() {
        tmpDir_0 = hdr.ipv4.srcAddr;
    }
    @hidden action pnaelimhdrcopydpdk141() {
        tmpDir_0 = hdr.ipv4.dstAddr;
    }
    @hidden action pnaelimhdrcopydpdk114() {
        eth_0.setInvalid();
    }
    @hidden table tbl_pnaelimhdrcopydpdk114 {
        actions = {
            pnaelimhdrcopydpdk114();
        }
        const default_action = pnaelimhdrcopydpdk114();
    }
    @hidden table tbl_pnaelimhdrcopydpdk139 {
        actions = {
            pnaelimhdrcopydpdk139();
        }
        const default_action = pnaelimhdrcopydpdk139();
    }
    @hidden table tbl_pnaelimhdrcopydpdk141 {
        actions = {
            pnaelimhdrcopydpdk141();
        }
        const default_action = pnaelimhdrcopydpdk141();
    }
    apply {
        tbl_pnaelimhdrcopydpdk114.apply();
        if (PNA_Direction_t.NET_TO_HOST == istd.direction) {
            tbl_pnaelimhdrcopydpdk139.apply();
        } else {
            tbl_pnaelimhdrcopydpdk141.apply();
        }
        if (hdr.ipv4.isValid()) {
            ipv4_da_lpm_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaelimhdrcopydpdk156() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnaelimhdrcopydpdk156 {
        actions = {
            pnaelimhdrcopydpdk156();
        }
        const default_action = pnaelimhdrcopydpdk156();
    }
    apply {
        tbl_pnaelimhdrcopydpdk156.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
