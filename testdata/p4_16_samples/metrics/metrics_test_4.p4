#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthD_t;
type    bit<48> EthT_t;

typedef bit<8>     CustomD_t;
type    bit<8>     CustomT_t;
typedef CustomD_t  CustomDD_t;
type    CustomD_t  CustomDT_t;
typedef CustomT_t  CustomTD_t;
type    CustomT_t  CustomTT_t;
typedef CustomDD_t CustomDDD_t;
type    CustomDD_t CustomDDT_t;
typedef CustomDT_t CustomDTD_t;
type    CustomDT_t CustomDTT_t;
typedef CustomTD_t CustomTDD_t;
type    CustomTD_t CustomTDT_t;
typedef CustomTT_t CustomTTD_t;
type    CustomTT_t CustomTTT_t;

struct nested_t {
    bit<8> f1;
    bit<8> f2;
}

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

header ipv4_new_t {
    bit<32> srcAddr;
    bit<32> dstAddr;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> length;
}

header stack_elem_t {
    bit<8> a;
    bit<8> b;
}

header bar_t {
    nested_t duo;
    bit<8>  val;
}

header_union u_hdr_t {
    ipv4_new_t v4;
    bar_t      s;
}

enum MyEnum {
    VALUE_C,
    VALUE_D
}

enum bit<8> MySerEnum {
    VALUE_A = 0,
    VALUE_B = 1
}

struct meta_t {
        MySerEnum   ser_enum_field; // enum bit<8>
        MyEnum      enum_field;     // enum
        bool        flag;           // Boolean
        int<32>     i32;            // signed integer
        error       err_field;      // error code
}

struct headers_t {
        ethernet_t  ethernet;
        custom_t    custom;
        ipv4_new_t  ipv4;
        stack_elem_t[2] stack_hdr;
        bar_t       bar;
        u_hdr_t     u_hdr;
}

//――――――――――――――――――――――――――――――――――――――――――――――――
// Parser
//――――――――――――――――――――――――――――――――――――――――――――――――

parser ParserImpl(
    packet_in               packet,
    out headers_t           hdr,
    inout meta_t            meta,
    inout standard_metadata_t standard_metadata)
{
    const bit<16> ETHERTYPE_CUSTOM = 16w0xDEAD;

    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_CUSTOM: parse_custom;
            default:            parse_done;
        }
    }
    state parse_custom {
        packet.extract(hdr.custom);
        transition parse_extra;
    }
    state parse_extra {
        packet.extract(hdr.ipv4);
        packet.extract(hdr.stack_hdr[0]);
        packet.extract(hdr.stack_hdr[1]);
        packet.extract(hdr.bar);

        hdr.u_hdr.v4 = hdr.ipv4;
        transition parse_done;
    }
    state parse_done {
        transition accept;
    }
}

control ingress(
    inout headers_t           hdr,
    inout meta_t              meta,
    inout standard_metadata_t standard_metadata)
{
    action set_output(bit<9> out_port) {
        standard_metadata.egress_spec = out_port;
    }
    action set_headers(
        bit<8>      e,    CustomD_t   ed,  CustomT_t   et,
        CustomDD_t  edd,  CustomDT_t  edt, CustomTD_t  etd,
        CustomTT_t  ett,  CustomDDD_t eddd,CustomDDT_t eddt,
        CustomDTD_t edtd, CustomDTT_t edtt,CustomTDD_t etdd,
        CustomTDT_t etdt, CustomTTD_t ettd,CustomTTT_t ettt)
    {
        headers_t tmp;
        tmp.custom.setValid();
        tmp.custom.e    = e;
        tmp.custom.ed   = ed;
        tmp.custom.et   = et;
        tmp.custom.edd  = edd;
        tmp.custom.edt  = edt;
        tmp.custom.etd  = etd;
        tmp.custom.ett  = ett;
        tmp.custom.eddd = eddd;
        tmp.custom.eddt = eddt;
        tmp.custom.edtd = edtd;
        tmp.custom.edtt = edtt;
        tmp.custom.etdd = etdd;
        tmp.custom.etdt = etdt;
        tmp.custom.ettd = ettd;
        tmp.custom.ettt = ettt;
        hdr.custom      = tmp.custom;
    }
    action my_drop() { }

    table custom_table {
        key = {
            hdr.ethernet.srcAddr : exact;
            hdr.ethernet.dstAddr : exact;
            hdr.custom.e         : exact;
            hdr.custom.etdd      : exact;
            hdr.custom.ettt      : exact;
            meta.i32             : exact;
            meta.flag            : exact;
            meta.ser_enum_field  : lpm;
            meta.enum_field      : exact;
            meta.err_field       : ternary;
        }
        actions = {
            set_output;
            set_headers;
            my_drop;
        }
        default_action = my_drop();
    }

    apply {
        if (hdr.custom.isValid()) {
            custom_table.apply();

            hdr.ipv4.ttl        = hdr.ipv4.ttl - 1;
            hdr.ipv4.protocol   = hdr.ipv4.protocol + 2;
            hdr.ipv4.srcAddr    = hdr.ipv4.srcAddr + 32w0x01010101;
            hdr.stack_hdr[0].a  = hdr.stack_hdr[0].a + hdr.stack_hdr[1].b;
            hdr.bar.val         = hdr.bar.duo.f2 - hdr.stack_hdr[0].a;
            hdr.u_hdr.v4.ttl    = hdr.ipv4.ttl >> 1;
            hdr.u_hdr.s.val     = (hdr.bar.val * 2) & 8w0x0F;
        }
    }
}

control egress(
    inout headers_t           hdr,
    inout meta_t              meta,
    inout standard_metadata_t standard_metadata)
{
    apply { }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.custom);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.stack_hdr[0]);
        packet.emit(hdr.stack_hdr[1]);
        packet.emit(hdr.bar);
        // emit the union's small branch for completeness
        packet.emit(hdr.u_hdr.s);
    }
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum(
            hdr.custom.isValid(),
            {
                hdr.custom.e,    hdr.custom.ed,
                hdr.custom.et,   hdr.custom.edd,
                hdr.custom.edt,  hdr.custom.etd,
                hdr.custom.ett,  hdr.custom.eddd,
                hdr.custom.eddt, hdr.custom.edtd,
                hdr.custom.edtt, hdr.custom.etdd,
                hdr.custom.etdt, hdr.custom.ettd,
                hdr.custom.ettt
            },
            hdr.custom.checksum,
            HashAlgorithm.csum16
        );
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum(
            hdr.custom.isValid(),
            {
                hdr.custom.e,    hdr.custom.ed,
                hdr.custom.et,   hdr.custom.edd,
                hdr.custom.edt,  hdr.custom.etd,
                hdr.custom.ett,  hdr.custom.eddd,
                hdr.custom.eddt, hdr.custom.edtd,
                hdr.custom.edtt, hdr.custom.etdd,
                hdr.custom.etdt, hdr.custom.ettd,
                hdr.custom.ettt
            },
            hdr.custom.checksum,
            HashAlgorithm.csum16
        );
    }
}

V1Switch(
    ParserImpl(),
    verifyChecksum(),
    ingress(),
    egress(),
    computeChecksum(),
    DeparserImpl()
) main;
