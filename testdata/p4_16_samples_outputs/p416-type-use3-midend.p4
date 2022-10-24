#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header custom_t {
    bit<8>  e;
    bit<8>  ed;
    bit<8>  et;
    bit<8>  edd;
    bit<8>  edt;
    bit<8>  etd;
    bit<8>  ett;
    bit<8>  eddd;
    bit<8>  eddt;
    bit<8>  edtd;
    bit<8>  edtt;
    bit<8>  etdd;
    bit<8>  etdt;
    bit<8>  ettd;
    bit<8>  ettt;
    bit<16> checksum;
}

struct meta_t {
}

struct headers_t {
    ethernet_t ethernet;
    custom_t   custom;
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    state start {
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

control ingress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.set_output") action set_output(@name("out_port") bit<9> out_port) {
        standard_metadata.egress_spec = out_port;
    }
    @name("ingress.set_headers") action set_headers(@name("e") bit<8> e_1, @name("ed") bit<8> ed_1, @name("et") bit<8> et_1, @name("edd") bit<8> edd_1, @name("edt") bit<8> edt_1, @name("etd") bit<8> etd_1, @name("ett") bit<8> ett_1, @name("eddd") bit<8> eddd_1, @name("eddt") bit<8> eddt_1, @name("edtd") bit<8> edtd_1, @name("edtt") bit<8> edtt_1, @name("etdd") bit<8> etdd_1, @name("etdt") bit<8> etdt_1, @name("ettd") bit<8> ettd_1, @name("ettt") bit<8> ettt_1) {
        hdr.custom.e = e_1;
        hdr.custom.ed = ed_1;
        hdr.custom.et = et_1;
        hdr.custom.edd = edd_1;
        hdr.custom.edt = edt_1;
        hdr.custom.etd = etd_1;
        hdr.custom.ett = ett_1;
        hdr.custom.eddd = eddd_1;
        hdr.custom.eddt = eddt_1;
        hdr.custom.edtd = edtd_1;
        hdr.custom.edtt = edtt_1;
        hdr.custom.etdd = etdd_1;
        hdr.custom.etdt = etdt_1;
        hdr.custom.ettd = ettd_1;
        hdr.custom.ettt = ettt_1;
    }
    @name("ingress.my_drop") action my_drop() {
    }
    @name("ingress.custom_table") table custom_table_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr");
            hdr.custom.e        : exact @name("hdr.custom.e");
            hdr.custom.ed       : exact @name("hdr.custom.ed");
            hdr.custom.et       : exact @name("hdr.custom.et");
            hdr.custom.edd      : exact @name("hdr.custom.edd");
            hdr.custom.eddt     : exact @name("hdr.custom.eddt");
            hdr.custom.edtd     : exact @name("hdr.custom.edtd");
            hdr.custom.edtt     : exact @name("hdr.custom.edtt");
            hdr.custom.etdd     : exact @name("hdr.custom.etdd");
            hdr.custom.etdt     : exact @name("hdr.custom.etdt");
            hdr.custom.ettd     : exact @name("hdr.custom.ettd");
            hdr.custom.ettt     : exact @name("hdr.custom.ettt");
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

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
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
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<8> f4;
    bit<8> f5;
    bit<8> f6;
    bit<8> f7;
    bit<8> f8;
    bit<8> f9;
    bit<8> f10;
    bit<8> f11;
    bit<8> f12;
    bit<8> f13;
    bit<8> f14;
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.custom.isValid(), (tuple_0){f0 = hdr.custom.e,f1 = hdr.custom.ed,f2 = hdr.custom.et,f3 = hdr.custom.edd,f4 = hdr.custom.edt,f5 = hdr.custom.etd,f6 = hdr.custom.ett,f7 = hdr.custom.eddd,f8 = hdr.custom.eddt,f9 = hdr.custom.edtd,f10 = hdr.custom.edtt,f11 = hdr.custom.etdd,f12 = hdr.custom.etdt,f13 = hdr.custom.ettd,f14 = hdr.custom.ettt}, hdr.custom.checksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.custom.isValid(), (tuple_0){f0 = hdr.custom.e,f1 = hdr.custom.ed,f2 = hdr.custom.et,f3 = hdr.custom.edd,f4 = hdr.custom.edt,f5 = hdr.custom.etd,f6 = hdr.custom.ett,f7 = hdr.custom.eddd,f8 = hdr.custom.eddt,f9 = hdr.custom.edtd,f10 = hdr.custom.edtt,f11 = hdr.custom.etdd,f12 = hdr.custom.etdt,f13 = hdr.custom.ettd,f14 = hdr.custom.ettt}, hdr.custom.checksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
