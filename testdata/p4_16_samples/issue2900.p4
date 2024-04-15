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

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct main_metadata_t {
    PortId_t dst_port;
}

bool RxPkt(in pna_main_input_metadata_t istd) {
    return istd.direction == PNA_Direction_t.NET_TO_HOST;
}
bool TxPkt(in pna_main_input_metadata_t istd) {
    return istd.direction == PNA_Direction_t.HOST_TO_NET;
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

control MainControlImpl(inout headers_t hdr, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    table clb_pinned_flows {
        key = {
            SelectByDirection(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr): exact @name("ipv4_addr_0") ;
            SelectByDirection(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr): exact @name("ipv4_addr_1") ;
            hdr.ipv4.protocol                                                    : exact;
        }
        actions = {
            NoAction;
        }
        const default_action = NoAction;
    }
    apply {
        if (TxPkt(istd)) {
            clb_pinned_flows.apply();
        } else {
            clb_pinned_flows.apply();
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

