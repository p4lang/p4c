#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthD_t;
@p4runtime_translation("p4.org/psa/v1/EthT_t" , 48) @custom_annotation_0 @custom_annotation_1("my annon") type bit<48> EthT_t;
typedef bit<8> CustomD_t;
type bit<8> CustomT_t;
typedef CustomD_t CustomDD_t;
type CustomD_t CustomDT_t;
typedef CustomT_t CustomTD_t;
type CustomT_t CustomTT_t;
typedef CustomDD_t CustomDDD_t;
type CustomDD_t CustomDDT_t;
typedef CustomDT_t CustomDTD_t;
type CustomDT_t CustomDTT_t;
typedef CustomTD_t CustomTDD_t;
type CustomTD_t CustomTDT_t;
typedef CustomTT_t CustomTTD_t;
type CustomTT_t CustomTTT_t;
header ethernet_t {
    EthD_t  dstAddr;
    EthT_t  srcAddr;
    bit<16> etherType;
}

header custom_t {
    bit<8>      e;
    CustomD_t   ed;
    CustomT_t   et;
    CustomDD_t  edd;
    CustomDT_t  edt;
    CustomTD_t  etd;
    CustomTT_t  ett;
    CustomDDD_t eddd;
    CustomDDT_t eddt;
    CustomDTD_t edtd;
    CustomDTT_t edtt;
    CustomTDD_t etdd;
    CustomTDT_t etdt;
    CustomTTD_t ettd;
    CustomTTT_t ettt;
    bit<16>     checksum;
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
    @name("ingress.set_headers") action set_headers(@name("e") bit<8> e_1, @name("ed") CustomD_t ed_1, @name("et") CustomT_t et_1, @name("edd") CustomDD_t edd_1, @name("edt") CustomDT_t edt_1, @name("etd") CustomTD_t etd_1, @name("ett") CustomTT_t ett_1, @name("eddd") CustomDDD_t eddd_1, @name("eddt") CustomDDT_t eddt_1, @name("edtd") CustomDTD_t edtd_1, @name("edtt") CustomDTT_t edtt_1, @name("etdd") CustomTDD_t etdd_1, @name("etdt") CustomTDT_t etdt_1, @name("ettd") CustomTTD_t ettd_1, @name("ettt") CustomTTT_t ettt_1) {
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

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum<tuple<bit<8>, bit<8>, CustomT_t, bit<8>, CustomDT_t, CustomT_t, CustomTT_t, bit<8>, CustomDDT_t, CustomDT_t, CustomDTT_t, CustomT_t, CustomTDT_t, CustomTT_t, CustomTTT_t>, bit<16>>(hdr.custom.isValid(), { hdr.custom.e, hdr.custom.ed, hdr.custom.et, hdr.custom.edd, hdr.custom.edt, hdr.custom.etd, hdr.custom.ett, hdr.custom.eddd, hdr.custom.eddt, hdr.custom.edtd, hdr.custom.edtt, hdr.custom.etdd, hdr.custom.etdt, hdr.custom.ettd, hdr.custom.ettt }, hdr.custom.checksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum<tuple<bit<8>, bit<8>, CustomT_t, bit<8>, CustomDT_t, CustomT_t, CustomTT_t, bit<8>, CustomDDT_t, CustomDT_t, CustomDTT_t, CustomT_t, CustomTDT_t, CustomTT_t, CustomTTT_t>, bit<16>>(hdr.custom.isValid(), { hdr.custom.e, hdr.custom.ed, hdr.custom.et, hdr.custom.edd, hdr.custom.edt, hdr.custom.etd, hdr.custom.ett, hdr.custom.eddd, hdr.custom.eddt, hdr.custom.edtd, hdr.custom.edtt, hdr.custom.etdd, hdr.custom.etdt, hdr.custom.ettd, hdr.custom.ettt }, hdr.custom.checksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
