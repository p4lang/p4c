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
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4_base);
        transition select(hdr.ipv4_base.ihl) {
            4w0x5: accept;
            default: parse_ipv4_options;
        }
    }
    state parse_ipv4_option_timestamp {
        bit<16> tmp16 = pkt.lookahead<bit<16>>();
        bit<8> tmp_len = tmp16[7:0];
        pkt.extract(hdr.ipv4_option_timestamp, (bit<32>)tmp_len * 8 - 16);
        transition accept;
    }
    state parse_ipv4_options {
        transition select(pkt.lookahead<bit<8>>()) {
            8w0x44 &&& 8w0xff: parse_ipv4_option_timestamp;
            default: accept;
        }
    }
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    action a1(bit<48> param) {
        hdr.ethernet.dstAddr = param;
    }
    action a2(bit<16> param) {
        hdr.ethernet.etherType = param;
    }
    table tbl {
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        actions = {
            NoAction;
            a2;
        }
    }
    table tbl2 {
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        actions = {
            NoAction;
            a1;
        }
    }
    apply {
        send_to_port((PortId_t)0);
        tbl.apply();
        tbl2.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4_base);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

