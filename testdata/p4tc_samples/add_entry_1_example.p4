#include <core.p4>
#include <tc/pna.p4>

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    @tc_type("macaddr") EthernetAddress srcAddr;
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

const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_NOW    = (ExpireTimeProfileId_t) 2;

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
    action send_nh( @tc_type("macaddr") bit<48> dmac, bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = dmac;
    }
    action next_hop() {
         add_entry(action_name = "send_nh",  // name of action
                 action_params = {hdr.ethernet.dstAddr, hdr.ethernet.srcAddr}, expire_time_profile_id = EXPIRE_TIME_PROFILE_NOW);

    }

    action dflt_route_drop() {
        drop_packet();
    }
    action drop() {
        drop_packet();
    }

    table ipv4_tbl_1 {
        key = {
            hdr.ipv4.dstAddr : exact @tc_type ("ipv4");
            istd.input_port : exact;
        }
        actions = {
            next_hop;
            send_nh;
            dflt_route_drop;
        }
        default_action = next_hop;
        add_on_miss = true;
    }

    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_tbl_1.apply();
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
