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
    bit<32> tmpDir;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    @hidden action pnadirectionmainparsererr79() {
        meta.tmpDir = hdr.ipv4.srcAddr;
    }
    @hidden action pnadirectionmainparsererr81() {
        meta.tmpDir = hdr.ipv4.dstAddr;
    }
    @hidden table tbl_pnadirectionmainparsererr79 {
        actions = {
            pnadirectionmainparsererr79();
        }
        const default_action = pnadirectionmainparsererr79();
    }
    @hidden table tbl_pnadirectionmainparsererr81 {
        actions = {
            pnadirectionmainparsererr81();
        }
        const default_action = pnadirectionmainparsererr81();
    }
    apply {
        if (PNA_Direction_t.NET_TO_HOST == istd.direction) {
            tbl_pnadirectionmainparsererr79.apply();
        } else {
            tbl_pnadirectionmainparsererr81.apply();
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
        transition select((bit<1>)(PNA_Direction_t.NET_TO_HOST == istd.direction)) {
            1w1: parse_ipv4_true;
            1w0: parse_ipv4_false;
            default: noMatch;
        }
    }
    state parse_ipv4_true {
        transition parse_ipv4_join;
    }
    state parse_ipv4_false {
        transition parse_ipv4_join;
    }
    state parse_ipv4_join {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.tmpDir") bit<32> tmpDir_1;
    @name("MainControlImpl.next_hop") action next_hop_0(@name("vport") bit<32> vport) {
        send_to_port(vport);
    }
    @name("MainControlImpl.default_route_drop") action default_route_drop_0() {
        drop_packet();
    }
    @name("MainControlImpl.ipv4_da_lpm") table ipv4_da_lpm {
        key = {
            tmpDir_1: lpm @name("ipv4_addr");
        }
        actions = {
            next_hop_0();
            default_route_drop_0();
        }
        const default_action = default_route_drop_0();
    }
    @hidden action pnadirectionmainparsererr138() {
        tmpDir_1 = hdr.ipv4.srcAddr;
    }
    @hidden action pnadirectionmainparsererr140() {
        tmpDir_1 = hdr.ipv4.dstAddr;
    }
    @hidden table tbl_pnadirectionmainparsererr138 {
        actions = {
            pnadirectionmainparsererr138();
        }
        const default_action = pnadirectionmainparsererr138();
    }
    @hidden table tbl_pnadirectionmainparsererr140 {
        actions = {
            pnadirectionmainparsererr140();
        }
        const default_action = pnadirectionmainparsererr140();
    }
    apply {
        if (PNA_Direction_t.NET_TO_HOST == istd.direction) {
            tbl_pnadirectionmainparsererr138.apply();
        } else {
            tbl_pnadirectionmainparsererr140.apply();
        }
        if (hdr.ipv4.isValid()) {
            ipv4_da_lpm.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadirectionmainparsererr155() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnadirectionmainparsererr155 {
        actions = {
            pnadirectionmainparsererr155();
        }
        const default_action = pnadirectionmainparsererr155();
    }
    apply {
        tbl_pnadirectionmainparsererr155.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
