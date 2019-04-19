#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthD_t;
@p4runtime_translation("p4.org/psa/v1/EthT_t" , 48) type bit<48> EthT_t;
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
    const bit<16> ETHERTYPE_CUSTOM = 16w0xdead;
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_CUSTOM: parse_custom;
            default: accept;
        }
    }
    state parse_custom {
        packet.extract(hdr.custom);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    action set_output(bit<9> out_port) {
        standard_metadata.egress_spec = out_port;
    }
    action set_headers(bit<8> e, CustomD_t ed, CustomT_t et, CustomDD_t edd, CustomDT_t edt, CustomTD_t etd, CustomTT_t ett, CustomDDD_t eddd, CustomDDT_t eddt, CustomDTD_t edtd, CustomDTT_t edtt, CustomTDD_t etdd, CustomTDT_t etdt, CustomTTD_t ettd, CustomTTT_t ettt) {
        hdr.custom.e = e;
        hdr.custom.ed = ed;
        hdr.custom.et = et;
        hdr.custom.edd = edd;
        hdr.custom.edt = edt;
        hdr.custom.etd = etd;
        hdr.custom.ett = ett;
        hdr.custom.eddd = eddd;
        hdr.custom.eddt = eddt;
        hdr.custom.edtd = edtd;
        hdr.custom.edtt = edtt;
        hdr.custom.etdd = etdd;
        hdr.custom.etdt = etdt;
        hdr.custom.ettd = ettd;
        hdr.custom.ettt = ettt;
    }
    action my_drop() {
    }
    table custom_table {
        key = {
            hdr.ethernet.srcAddr: exact;
            hdr.ethernet.dstAddr: exact;
            hdr.custom.e        : exact;
            hdr.custom.ed       : exact;
            hdr.custom.et       : exact;
            hdr.custom.edd      : exact;
            hdr.custom.eddt     : exact;
            hdr.custom.edtd     : exact;
            hdr.custom.edtt     : exact;
            hdr.custom.etdd     : exact;
            hdr.custom.etdt     : exact;
            hdr.custom.ettd     : exact;
            hdr.custom.ettt     : exact;
        }
        actions = {
            set_output;
            set_headers;
            my_drop;
        }
        default_action = my_drop;
    }
    apply {
        if (hdr.custom.isValid()) {
            custom_table.apply();
        }
    }
}

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.custom);
    }
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum(hdr.custom.isValid(), { hdr.custom.e, hdr.custom.ed, hdr.custom.et, hdr.custom.edd, hdr.custom.edt, hdr.custom.etd, hdr.custom.ett, hdr.custom.eddd, hdr.custom.eddt, hdr.custom.edtd, hdr.custom.edtt, hdr.custom.etdd, hdr.custom.etdt, hdr.custom.ettd, hdr.custom.ettt }, hdr.custom.checksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum(hdr.custom.isValid(), { hdr.custom.e, hdr.custom.ed, hdr.custom.et, hdr.custom.edd, hdr.custom.edt, hdr.custom.etd, hdr.custom.ett, hdr.custom.eddd, hdr.custom.eddt, hdr.custom.edtd, hdr.custom.edtt, hdr.custom.etdd, hdr.custom.etdt, hdr.custom.ettd, hdr.custom.ettt }, hdr.custom.checksum, HashAlgorithm.csum16);
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

