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
    @tc_type ("ipv4") bit<32> srcAddr;
    @tc_type ("ipv4") bit<32> dstAddr;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

// Masks of the bit positions of some bit flags within the TCP flags field.
const bit<8> TCP_URG_MASK = 0x20;
const bit<8> TCP_ACK_MASK = 0x10;
const bit<8> TCP_PSH_MASK = 0x08;
const bit<8> TCP_RST_MASK = 0x04;
const bit<8> TCP_SYN_MASK = 0x02;
const bit<8> TCP_FIN_MASK = 0x01;

const PortId_t hport = (PortId_t) 0;
const PortId_t nport = (PortId_t) 1;

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
    tcp_t      tcp;
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
        transition select(hdr.ipv4.protocol) {
            6       : parse_tcp;
            default : accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }
}

control MainControlImpl(
    inout headers_t hdr,                 // from main parser
    inout main_metadata_t user_meta,     // from main parser, to "next block"
    in    pna_main_input_metadata_t istd,
    inout pna_main_output_metadata_t ostd)
{
    bool do_add_on_miss;
    bool update_aging_info;
    bool update_expire_time;
    ExpireTimeProfileId_t new_expire_time_profile_id;

    action next_hop(PortId_t vport) {
        hdr.ipv4.version = 1;
        send_to_port(vport);
    }
    action default_route_drop() {
        if (!(hdr.ipv4.protocol == 6)) {
            drop_packet();
        }
    }
    action sendtoport() {
        send_to_port(istd.input_port);
    }
    action drop() {
        if ((hdr.ipv4.protocol == 6) || (hdr.ipv4.version > 1) && (hdr.ipv4.ihl <= 2)) {
            drop_packet();
        }
    }
    action tcp_syn_packet () {
        do_add_on_miss = true;
        update_aging_info = true;
        update_expire_time = true;
    }
    action tcp_fin_or_rst_packet () {
        update_aging_info = true;
        update_expire_time = true;
    }
    action tcp_other_packets () {
        do_add_on_miss = true;
        update_expire_time = true;
    }

    table ipv4_tbl_1 {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            @tableonly next_hop;
            default_route_drop;
        }
        default_action = default_route_drop;
    }
    table ipv4_tbl_2 {
        key = {
            hdr.ipv4.dstAddr  : exact;
            hdr.ipv4.srcAddr  : exact;
            hdr.ipv4.protocol : exact;
        }
        actions = {
            next_hop;
            @defaultonly drop;
        }
        default_action = drop;
    }
    table ipv4_tbl_3 {
        key = {
            hdr.ipv4.dstAddr : exact;
            hdr.ipv4.srcAddr : exact;
            hdr.ipv4.flags   : exact;
        }
        actions = {
            sendtoport;
            drop;
        }
        size = 1024;
    }
    table ipv4_tbl_4 {
        key = {
            hdr.ipv4.dstAddr    : exact;
            hdr.ipv4.srcAddr    : exact;
            hdr.ipv4.fragOffset : exact;
        }
        actions = {
            sendtoport;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }
    table ipv4_tbl_5 {
        key = {
            hdr.ipv4.fragOffset : exact;
        }
        actions = {
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }
    table set_ct_options {
        key = {
            hdr.tcp.flags : ternary;
        }
        actions = {
            tcp_syn_packet;
            tcp_fin_or_rst_packet;
            tcp_other_packets;
        }
        const default_action = tcp_other_packets;
    }
    table set_all_options {
        key = {
            hdr.ipv4.srcAddr    : exact;
            hdr.tcp.srcPort     : exact;
            hdr.ipv4.fragOffset : exact;
            hdr.ipv4.flags      : exact;
        }
        actions = {
            next_hop;
            default_route_drop;
            tcp_syn_packet;
            tcp_fin_or_rst_packet;
            tcp_other_packets;
            sendtoport;
            drop;
            NoAction;
        }
        const entries = {
            (0x1000, 0x10, 10, 1) : tcp_syn_packet;
            (0x2000, 0x20, 20, 2) : tcp_fin_or_rst_packet;
            (0x3000, 0x30, 30, 3) : tcp_fin_or_rst_packet;
            (0x4000, 0x40, 40, 4) : default_route_drop;
            (0x5000, 0x50, 50, 5) : next_hop(hport);
            (0x6000, 0x60, 60, 6) : sendtoport();
        }
        const default_action = drop;
    }

    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_tbl_1.apply();
            ipv4_tbl_2.apply();
            ipv4_tbl_3.apply();
            ipv4_tbl_4.apply();
            ipv4_tbl_5.apply();
            set_ct_options.apply();
            set_all_options.apply();
        }
    }
}

control MainDeparserImpl(
    packet_out pkt,
    inout headers_t hdr,                    // from main control
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
