#include <core.p4>
#include <pna.p4>

typedef bit<48> EthernetAddress;
const bit<8> ETHERNET_HEADER_LEN = 8w14;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
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

struct empty_metadata_t {
}

struct main_metadata_t {
    bit<32> port_out;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

const bit<32> IPV4_HOST_TABLE_MAX_ENTRIES = 32w1024;
parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    PNA_MeterColor_t color_out;
    PNA_MeterColor_t out1;
    PNA_MeterColor_t color_in = PNA_MeterColor_t.RED;
    DirectMeter(PNA_MeterType_t.PACKETS) meter0;
    action send() {
        out1 = meter0.execute(color_in, 32w1024);
        user_meta.port_out = (out1 == PNA_MeterColor_t.GREEN ? 32w1 : 32w0);
    }
    table ipv4_host {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            send();
        }
        const default_action = send();
        size = 32w1024;
        pna_direct_meter = meter0;
    }
    apply {
        color_out = meter0.execute(color_in, 32w1024);
        user_meta.port_out = (color_out == PNA_MeterColor_t.GREEN ? 32w1 : 32w0);
        ipv4_host.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

