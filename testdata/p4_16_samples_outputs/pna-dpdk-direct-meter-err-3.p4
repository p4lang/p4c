#include <core.p4>
#include <pna.p4>

typedef bit<48> EthernetAddress;
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

typedef bit<48> ByteCounter_t;
typedef bit<32> PacketCounter_t;
typedef bit<80> PacketByteCounter_t;
const bit<32> NUM_PORTS = 512;
struct main_metadata_t {
    bit<32> port_out;
    bit<32> port_out1;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    PNA_MeterColor_t color_out;
    PNA_MeterColor_t out1;
    PNA_MeterColor_t color_in = PNA_MeterColor_t.RED;
    DirectMeter(PNA_MeterType_t.BYTES) meter0;
    DirectMeter(PNA_MeterType_t.BYTES) meter1;
    action next_hop(PortId_t oport) {
        out1 = meter0.execute(color_in, 32w1024);
        user_meta.port_out = (out1 == PNA_MeterColor_t.GREEN ? 32w1 : 32w0);
        color_out = meter1.execute(color_in, 32w1024);
        user_meta.port_out1 = (color_out == PNA_MeterColor_t.GREEN ? 32w1 : 32w0);
        send_to_port(oport);
    }
    action default_route_drop() {
        out1 = meter0.execute(color_in, 32w1024);
        user_meta.port_out = (out1 == PNA_MeterColor_t.GREEN ? 32w1 : 32w0);
        drop_packet();
    }
    action default_route_drop1() {
        out1 = meter1.execute(color_in, 32w1024);
        user_meta.port_out = (out1 == PNA_MeterColor_t.GREEN ? 32w1 : 32w0);
        drop_packet();
    }
    table ipv4_da1 {
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        actions = {
            next_hop;
            default_route_drop;
        }
        default_action = default_route_drop;
        pna_direct_meter = meter0;
    }
    table ipv4_da {
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        actions = {
            next_hop;
            default_route_drop1;
        }
        default_action = default_route_drop1;
        pna_direct_meter = meter1;
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_da.apply();
            ipv4_da1.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
