#include <core.p4>
#include <dpdk/pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header custom_t {
    bit<1>   padding;
    bit<1>   f1;
    bit<2>   f2;
    bit<4>   f4;
    bit<8>   f8;
    bit<16>  f16;
    bit<32>  f32;
    bit<64>  f64;
    bit<128> f128;
}

struct main_metadata_t {
}

struct headers_t {
    ethernet_t ethernet;
    custom_t   custom;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x8ff: parse_custom;
            default: accept;
        }
    }
    state parse_custom {
        pkt.extract<custom_t>(hdr.custom);
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.a1") action a1(@name("x") bit<73> x) {
        hdr.custom.f128 = (bit<128>)x;
    }
    @name("MainControlImpl.t1") table t1_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr");
        }
        actions = {
            @tableonly a1();
            @defaultonly NoAction_1();
        }
        size = 512;
        const default_action = NoAction_1();
    }
    apply {
        if (hdr.custom.isValid()) {
            t1_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdk_128bit_odd_size112() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<custom_t>(hdr.custom);
    }
    @hidden table tbl_pnadpdk_128bit_odd_size112 {
        actions = {
            pnadpdk_128bit_odd_size112();
        }
        const default_action = pnadpdk_128bit_odd_size112();
    }
    apply {
        tbl_pnadpdk_128bit_odd_size112.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
