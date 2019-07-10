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
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
            hdr.custom.e        : exact @name("hdr.custom.e") ;
            hdr.custom.ed       : exact @name("hdr.custom.ed") ;
            hdr.custom.et       : exact @name("hdr.custom.et") ;
            hdr.custom.edd      : exact @name("hdr.custom.edd") ;
            hdr.custom.eddt     : exact @name("hdr.custom.eddt") ;
            hdr.custom.edtd     : exact @name("hdr.custom.edtd") ;
            hdr.custom.edtt     : exact @name("hdr.custom.edtt") ;
            hdr.custom.etdd     : exact @name("hdr.custom.etdd") ;
            hdr.custom.etdt     : exact @name("hdr.custom.etdt") ;
            hdr.custom.ettd     : exact @name("hdr.custom.ettd") ;
            hdr.custom.ettt     : exact @name("hdr.custom.ettt") ;
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

