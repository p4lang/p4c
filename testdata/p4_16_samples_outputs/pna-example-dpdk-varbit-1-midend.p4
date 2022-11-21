#include <core.p4>
#include <dpdk/pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_base_t {
    bit<8>  version_ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<16> flags_fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header ipv4_option_timestamp_t {
    bit<8>      value;
    bit<8>      len;
    varbit<304> data;
}

header option_t {
    bit<8> value;
    bit<8> len;
}

struct main_metadata_t {
}

struct headers_t {
    ethernet_t              ethernet;
    ipv4_base_t             ipv4_base;
    ipv4_option_timestamp_t ipv4_option_timestamp;
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    @name("MainParserImpl.tmp_0") bit<8> tmp_0;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_base_t>(hdr.ipv4_base);
        transition select(hdr.ipv4_base.version_ihl) {
            8w0x45: accept;
            default: parse_ipv4_options;
        }
    }
    state parse_ipv4_option_timestamp {
        pkt.lookahead<bit<16>>();
        pkt.extract<ipv4_option_timestamp_t>(hdr.ipv4_option_timestamp, 32w16);
        transition accept;
    }
    state parse_ipv4_options {
        tmp_0 = pkt.lookahead<bit<8>>();
        transition select(tmp_0) {
            8w0x44: parse_ipv4_option_timestamp;
            default: accept;
        }
    }
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("MainControlImpl.a1") action a1(@name("param") bit<48> param) {
        hdr.ethernet.dstAddr = param;
    }
    @name("MainControlImpl.a2") action a2(@name("param") bit<16> param_2) {
        hdr.ethernet.etherType = param_2;
    }
    @name("MainControlImpl.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            a2();
        }
        default_action = NoAction_1();
    }
    @name("MainControlImpl.tbl2") table tbl2_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_2();
            a1();
        }
        default_action = NoAction_2();
    }
    @hidden action pnaexampledpdkvarbit1l124() {
        send_to_port(32w0);
    }
    @hidden table tbl_pnaexampledpdkvarbit1l124 {
        actions = {
            pnaexampledpdkvarbit1l124();
        }
        const default_action = pnaexampledpdkvarbit1l124();
    }
    apply {
        tbl_pnaexampledpdkvarbit1l124.apply();
        tbl_0.apply();
        tbl2_0.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexampledpdkvarbit1l137() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_base_t>(hdr.ipv4_base);
    }
    @hidden table tbl_pnaexampledpdkvarbit1l137 {
        actions = {
            pnaexampledpdkvarbit1l137();
        }
        const default_action = pnaexampledpdkvarbit1l137();
    }
    apply {
        tbl_pnaexampledpdkvarbit1l137.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
