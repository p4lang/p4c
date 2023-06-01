#include <core.p4>
#include <tc/pna.p4>

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

//////////////////////////////////////////////////////////////////////
// Struct types for holding user-defined collections of headers and
// metadata in the P4 developer's program.
//
// Note: The names of these struct types are completely up to the P4
// developer, as are their member fields, with the only restriction
// being that the structs intended to contain headers should only
// contain members whose types are header, header stack, or
// header_union.
//////////////////////////////////////////////////////////////////////

struct main_metadata_t {
    // empty for this skeleton
}

// User-defined struct containing all of those headers parsed in the
// main parser.
struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser MainParserImpl(
    packet_in pkt,
    out   headers_t hdr,
    inout main_metadata_t main_meta,
    in    pna_main_parser_input_metadata_t istd)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800  : parse_ipv4;
            default : accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

control MainControlImpl(
    inout headers_t hdr,                 // from main parser
    inout main_metadata_t user_meta,     // from main parser, to "next block"
    in    pna_main_input_metadata_t istd,
    inout pna_main_output_metadata_t ostd)
{
    action next_hop(PortId_t vport) {
        send_to_port(vport);
    }
    action default_route_drop() {
        drop_packet();
    }
    action drop() {
        drop_packet();
    }

    table ipv4_tbl_1 {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            next_hop;
            default_route_drop;
        }
        default_action = default_route_drop;
    }
    table ipv4_tbl_2 {
        key = {
            hdr.ipv4.dstAddr  : exact;
            hdr.ipv4.srcAddr  : ternary;
            hdr.ipv4.protocol : range;
        }
        actions = {
            next_hop;
            drop;
        }
        default_action = drop;
    }
    table ipv4_tbl_3 {
        key = {
            hdr.ipv4.srcAddr : optional;
        }
        actions = {
            next_hop;
            drop;
        }
        default_action = drop;
    }
    table ipv4_tbl_4 {
        key = {
            hdr.ipv4.dstAddr  : exact;
            hdr.ipv4.srcAddr  : exact;
            hdr.ipv4.protocol : lpm;
        }
        actions = {
            next_hop;
            drop;
        }
        default_action = drop;
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_tbl_1.apply();
            ipv4_tbl_2.apply();
            ipv4_tbl_3.apply();
            ipv4_tbl_4.apply();
        }
    }
}

control MainDeparserImpl(
    packet_out pkt,
    in headers_t hdr,                    // from main control
    in main_metadata_t user_meta,        // from main control
    in pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
    }
}

// BEGIN:Package_Instantiation_Example
PNA_NIC(
    MainParserImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;
// END:Package_Instantiation_Example
