#include <core.p4>
#include <pna.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_base_t {
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

header ipv4_option_timestamp_t {
    bit<8>      value;
    bit<8>      len;
    varbit<304> data;
}

struct main_metadata_t {
}

struct headers_t {
    ethernet_t              ethernet;
    ipv4_base_t             ipv4_base;
    ipv4_option_timestamp_t ipv4_option_timestamp;
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    @name("MainParserImpl.tmp16") bit<16> tmp16_0;
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
        transition select(hdr.ipv4_base.ihl) {
            4w0x5: accept;
            default: parse_ipv4_options;
        }
    }
    state parse_ipv4_option_timestamp {
        tmp16_0 = pkt.lookahead<bit<16>>();
        pkt.extract<ipv4_option_timestamp_t>(hdr.ipv4_option_timestamp, ((bit<32>)tmp16_0[7:0] << 3) + 32w4294967280);
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
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        actions = {
            NoAction_1();
            a2();
        }
        default_action = NoAction_1();
    }
    @name("MainControlImpl.tbl2") table tbl2_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        actions = {
            NoAction_2();
            a1();
        }
        default_action = NoAction_2();
    }
    @hidden action pnaexampledpdkvarbit122() {
        send_to_port(32w0);
    }
    @hidden table tbl_pnaexampledpdkvarbit122 {
        actions = {
            pnaexampledpdkvarbit122();
        }
        const default_action = pnaexampledpdkvarbit122();
    }
    apply {
        tbl_pnaexampledpdkvarbit122.apply();
        tbl_0.apply();
        tbl2_0.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexampledpdkvarbit135() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_base_t>(hdr.ipv4_base);
    }
    @hidden table tbl_pnaexampledpdkvarbit135 {
        actions = {
            pnaexampledpdkvarbit135();
        }
        const default_action = pnaexampledpdkvarbit135();
    }
    apply {
        tbl_pnaexampledpdkvarbit135.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

