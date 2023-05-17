#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct struct1_t {
    bit<7> x;
    bit<9> y;
}

header custom_t {
    bit<48> _addr00;
    bit<48> _addr11;
    bit<48> _addr22;
    bit<8>  _e3;
    bit<8>  _e04;
    bit<8>  _e15;
    bit<8>  _e26;
    bit<8>  _e007;
    bit<8>  _e018;
    bit<8>  _e029;
    bit<8>  _e1010;
    bit<8>  _e1111;
    bit<8>  _e1212;
    bit<8>  _e2013;
    bit<8>  _e2114;
    bit<8>  _e2215;
    bit<8>  _e00116;
    bit<8>  _e00217;
    bit<8>  _e10118;
    bit<8>  _e10219;
    bit<8>  _e20120;
    bit<8>  _e20221;
    bit<8>  _e22022;
    bit<8>  _e002001023;
    bit<8>  _e002002024;
    bit<7>  _my_nested_struct1_x25;
    bit<9>  _my_nested_struct1_y26;
    bit<16> _checksum27;
    int<8>  _s028;
}

@controller_header("packet_in") header packet_in_header_t {
    bit<8> punt_reason;
}

@controller_header("packet_out") header packet_out_header_t {
    bit<48> addr0;
    bit<48> addr1;
    bit<48> addr2;
    bit<8>  e;
    bit<8>  e0;
    bit<8>  e1;
    bit<8>  e2;
    bit<8>  e00;
    bit<8>  e01;
    bit<8>  e02;
    bit<8>  e10;
    bit<8>  e11;
    bit<8>  e12;
    bit<8>  e20;
    bit<8>  e21;
    bit<8>  e22;
    bit<8>  e001;
    bit<8>  e002;
    bit<8>  e101;
    bit<8>  e102;
    bit<8>  e201;
    bit<8>  e202;
    bit<8>  e220;
    bit<8>  e0020010;
    bit<8>  e0020020;
}

struct headers_t {
    packet_in_header_t  packet_in;
    packet_out_header_t packet_out;
    ethernet_t          ethernet;
    custom_t            custom;
}

struct valueset1_t {
    bit<48> addr0;
    bit<8>  e;
    bit<8>  e0;
    bit<8>  e00;
}

struct metadata_t {
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ParserImpl.valueset1") value_set<valueset1_t>(4) valueset1_0;
    state start {
        transition select(stdmeta.ingress_port) {
            9w0: parse_packet_out_header;
            default: parse_ethernet;
        }
    }
    state parse_packet_out_header {
        packet.extract<packet_out_header_t>(hdr.packet_out);
        transition select(hdr.packet_out.addr0, hdr.packet_out.e, hdr.packet_out.e0, hdr.packet_out.e00) {
            valueset1_0: accept;
            default: parse_ethernet;
        }
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0xdead: parse_custom;
            default: accept;
        }
    }
    state parse_custom {
        packet.extract<custom_t>(hdr.custom);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingress.set_output") action set_output(@name("out_port") bit<9> out_port) {
        stdmeta.egress_spec = out_port;
    }
    @name("ingress.set_headers") action set_headers(@name("addr0") bit<48> addr0_1, @name("addr1") bit<48> addr1_1, @name("addr2") bit<48> addr2_1, @name("e") bit<8> e_1, @name("e0") bit<8> e0_1, @name("e1") bit<8> e1_1, @name("e2") bit<8> e2_1, @name("e00") bit<8> e00_1, @name("e01") bit<8> e01_1, @name("e02") bit<8> e02_1, @name("e10") bit<8> e10_1, @name("e11") bit<8> e11_1, @name("e12") bit<8> e12_1, @name("e20") bit<8> e20_1, @name("e21") bit<8> e21_1, @name("e22") bit<8> e22_1, @name("e001") bit<8> e001_1, @name("e002") bit<8> e002_1, @name("e101") bit<8> e101_1, @name("e102") bit<8> e102_1, @name("e201") bit<8> e201_1, @name("e202") bit<8> e202_1, @name("e220") bit<8> e220_1, @name("e0020010") bit<8> e0020010_1, @name("e0020020") bit<8> e0020020_1, @name("s0") int<8> s0_1) {
        hdr.custom._addr00 = addr0_1;
        hdr.custom._addr11 = addr1_1;
        hdr.custom._addr22 = addr2_1;
        hdr.custom._e3 = e_1;
        hdr.custom._e04 = e0_1;
        hdr.custom._e15 = e1_1;
        hdr.custom._e26 = e2_1;
        hdr.custom._e007 = e00_1;
        hdr.custom._e018 = e01_1;
        hdr.custom._e029 = e02_1;
        hdr.custom._e1010 = e10_1;
        hdr.custom._e1111 = e11_1;
        hdr.custom._e1212 = e12_1;
        hdr.custom._e2013 = e20_1;
        hdr.custom._e2114 = e21_1;
        hdr.custom._e2215 = e22_1;
        hdr.custom._e00116 = e001_1;
        hdr.custom._e00217 = e002_1;
        hdr.custom._e10118 = e101_1;
        hdr.custom._e10219 = e102_1;
        hdr.custom._e20120 = e201_1;
        hdr.custom._e20221 = e202_1;
        hdr.custom._e22022 = e220_1;
        hdr.custom._e002001023 = e0020010_1;
        hdr.custom._e002002024 = e0020020_1;
        hdr.custom._s028 = s0_1;
    }
    @name("ingress.my_drop") action my_drop() {
    }
    @name("ingress.custom_table") table custom_table_0 {
        key = {
            hdr.custom._addr00    : exact @name("hdr.custom.addr0");
            hdr.custom._addr11    : exact @name("hdr.custom.addr1");
            hdr.custom._addr22    : exact @name("hdr.custom.addr2");
            hdr.custom._e3        : exact @name("hdr.custom.e");
            hdr.custom._e04       : exact @name("hdr.custom.e0");
            hdr.custom._e15       : exact @name("hdr.custom.e1");
            hdr.custom._e26       : exact @name("hdr.custom.e2");
            hdr.custom._e007      : exact @name("hdr.custom.e00");
            hdr.custom._e018      : exact @name("hdr.custom.e01");
            hdr.custom._e029      : exact @name("hdr.custom.e02");
            hdr.custom._e1010     : exact @name("hdr.custom.e10");
            hdr.custom._e1111     : exact @name("hdr.custom.e11");
            hdr.custom._e1212     : exact @name("hdr.custom.e12");
            hdr.custom._e2013     : exact @name("hdr.custom.e20");
            hdr.custom._e2114     : exact @name("hdr.custom.e21");
            hdr.custom._e2215     : exact @name("hdr.custom.e22");
            hdr.custom._e00116    : exact @name("hdr.custom.e001");
            hdr.custom._e00217    : exact @name("hdr.custom.e002");
            hdr.custom._e10118    : exact @name("hdr.custom.e101");
            hdr.custom._e10219    : exact @name("hdr.custom.e102");
            hdr.custom._e20120    : exact @name("hdr.custom.e201");
            hdr.custom._e20221    : exact @name("hdr.custom.e202");
            hdr.custom._e22022    : exact @name("hdr.custom.e220");
            hdr.custom._e002001023: exact @name("hdr.custom.e0020010");
            hdr.custom._e002002024: exact @name("hdr.custom.e0020020");
            hdr.custom._s028      : exact @name("hdr.custom.s0");
        }
        actions = {
            set_output();
            set_headers();
            my_drop();
        }
        default_action = my_drop();
    }
    apply {
        if (hdr.custom.isValid()) {
            custom_table_0.apply();
        }
    }
}

control egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<custom_t>(hdr.custom);
    }
}

struct tuple_0 {
    bit<48> f0;
    bit<48> f1;
    bit<48> f2;
    bit<8>  f3;
    bit<8>  f4;
    bit<8>  f5;
    bit<8>  f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<8>  f9;
    bit<8>  f10;
    bit<8>  f11;
    bit<8>  f12;
    bit<8>  f13;
    bit<8>  f14;
    bit<8>  f15;
    bit<8>  f16;
    bit<8>  f17;
    bit<8>  f18;
    bit<8>  f19;
    bit<8>  f20;
    bit<8>  f21;
    bit<8>  f22;
    bit<8>  f23;
    bit<8>  f24;
    int<8>  f25;
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.custom.isValid(), (tuple_0){f0 = hdr.custom._addr00,f1 = hdr.custom._addr11,f2 = hdr.custom._addr22,f3 = hdr.custom._e3,f4 = hdr.custom._e04,f5 = hdr.custom._e15,f6 = hdr.custom._e26,f7 = hdr.custom._e007,f8 = hdr.custom._e018,f9 = hdr.custom._e029,f10 = hdr.custom._e1010,f11 = hdr.custom._e1111,f12 = hdr.custom._e1212,f13 = hdr.custom._e2013,f14 = hdr.custom._e2114,f15 = hdr.custom._e2215,f16 = hdr.custom._e00116,f17 = hdr.custom._e00217,f18 = hdr.custom._e10118,f19 = hdr.custom._e10219,f20 = hdr.custom._e20120,f21 = hdr.custom._e20221,f22 = hdr.custom._e22022,f23 = hdr.custom._e002001023,f24 = hdr.custom._e002002024,f25 = hdr.custom._s028}, hdr.custom._checksum27, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.custom.isValid(), (tuple_0){f0 = hdr.custom._addr00,f1 = hdr.custom._addr11,f2 = hdr.custom._addr22,f3 = hdr.custom._e3,f4 = hdr.custom._e04,f5 = hdr.custom._e15,f6 = hdr.custom._e26,f7 = hdr.custom._e007,f8 = hdr.custom._e018,f9 = hdr.custom._e029,f10 = hdr.custom._e1010,f11 = hdr.custom._e1111,f12 = hdr.custom._e1212,f13 = hdr.custom._e2013,f14 = hdr.custom._e2114,f15 = hdr.custom._e2215,f16 = hdr.custom._e00116,f17 = hdr.custom._e00217,f18 = hdr.custom._e10118,f19 = hdr.custom._e10219,f20 = hdr.custom._e20120,f21 = hdr.custom._e20221,f22 = hdr.custom._e22022,f23 = hdr.custom._e002001023,f24 = hdr.custom._e002002024,f25 = hdr.custom._s028}, hdr.custom._checksum27, HashAlgorithm.csum16);
    }
}

V1Switch<headers_t, metadata_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
