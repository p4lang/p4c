#include <core.p4>
#include "pna.p4"


typedef bit<48>  EthernetAddress;

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

// BEGIN:Counter_Example_Part1
typedef bit<48> ByteCounter_t;
typedef bit<32> PacketCounter_t;
typedef bit<80> PacketByteCounter_t;

const bit<32> NUM_PORTS = 512;
// END:Counter_Example_Part1

struct main_metadata_t {
}

struct headers_t {
    ethernet_t       ethernet;
    ipv4_t           ipv4;
}

control PreControlImpl(
    in    headers_t  hdr,
    inout main_metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
    }
}

parser MainParserImpl(
    packet_in pkt,
    out   headers_t       hdr,
    inout main_metadata_t main_meta,
    in    pna_main_parser_input_metadata_t istd)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

// BEGIN:Counter_Example_Part2
control MainControlImpl(
    inout headers_t       hdr,           // from main parser
    inout main_metadata_t user_meta,     // from main parser, to "next block"
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    DirectCounter<PacketByteCounter_t>(PNA_CounterType_t.PACKETS_AND_BYTES)
        per_prefix_pkt_byte_count;
    DirectCounter<PacketByteCounter_t>(PNA_CounterType_t.PACKETS_AND_BYTES)
        per_prefix_pkt_byte_count1;

    action next_hop(PortId_t oport) {
        per_prefix_pkt_byte_count.count();
        send_to_port(oport);
    }
    action default_route_drop() {
        per_prefix_pkt_byte_count.count();
        drop_packet();
    }
    table ipv4_tbl1 {
        key = { hdr.ipv4.dstAddr: exact; }
        actions = {
            default_route_drop;
        }
        default_action = default_route_drop;
        // table ipv4_tbl1 owns this DirectCounter instance
        pna_direct_counter = per_prefix_pkt_byte_count1;
    }
    table ipv4_tbl {
        key = { hdr.ipv4.dstAddr: lpm; }
        actions = {
            next_hop;
            default_route_drop;
        }
        default_action = default_route_drop;
        // table ipv4_tbl owns this DirectCounter instance
        pna_direct_counter = per_prefix_pkt_byte_count;
    }
    apply {
        if (hdr.ipv4.isValid()) {
            per_prefix_pkt_byte_count.count();
            ipv4_tbl.apply();
            ipv4_tbl1.apply();
        }
    }
}
control MainDeparserImpl(
    packet_out pkt,
    in    headers_t hdr,                // from main control
    in    main_metadata_t user_meta,    // from main control
    in    pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
    }
}

// BEGIN:Package_Instantiation_Example
PNA_NIC(
    MainParserImpl(),
    PreControlImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;
// END:Package_Instantiation_Example
