#include <core.p4>
#include <tc/pna.p4>

typedef bit<48>  EthernetAddress;

header ethernet_t {
    @tc_type("macaddr") EthernetAddress dstAddr;
    @tc_type("macaddr") EthernetAddress srcAddr;
    bit<16> etherType;
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

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLength;
    bit<8>   nextHeader;
    bit<8>   hopLimit;
    @tc_type ("ipv6") bit<128> srcAddr;
    @tc_type ("ipv6") bit<128> dstAddr;
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
    ipv6_t     ipv6;
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
            0x86DD: parse_ipv6;
            default: accept;
        }
    }

    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition accept;
    }
}

control MainControlImpl(
    inout headers_t hdr,                 // from main parser
    inout main_metadata_t user_meta,     // from main parser, to "next block"
    in    pna_main_input_metadata_t istd,
    inout pna_main_output_metadata_t ostd)
{

    action set_dst(bit<128> addr6) {
        hdr.ipv6.dstAddr = addr6;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit - 1;  // validate number of matches
    }

    table tbl_default {
        key = {
            hdr.ipv6.srcAddr : exact;
        }
        actions = { set_dst; NoAction; }
        default_action = set_dst(128w0xffff_1111_2222_3333_4444_5555_6666_aaaa);
        size = 100;
    }

    apply {
        tbl_default.apply();
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
        pkt.emit(hdr.ipv6);
    }
}

// BEGIN:Package_Instantiation_Example
PNA_NIC(
    MainParserImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;
// END:Package_Instantiation_Example
