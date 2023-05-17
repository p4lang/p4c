#include <core.p4>
#include <pna.p4>

typedef bit<48>  EthernetAddress;
const bit<8> ETHERNET_HEADER_LEN = 14;

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
    bit<32> data;
}


struct headers_t {
    ethernet_t ethernet;
    ipv4_t ipv4;
}

typedef bit<48> ByteCounter_t;
typedef bit<32> PacketCounter_t;
typedef bit<80> PacketByteCounter_t;
typedef bit<16>  FlowIdx_t;
const bit<32> IPV4_HOST_TABLE_MAX_ENTRIES = 1024;

parser MainParserImpl(
    packet_in pkt,
    out   headers_t hdr,
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

control PreControlImpl(
        in    headers_t  hdr,
        inout main_metadata_t meta,
        in    pna_pre_input_metadata_t  istd,
        inout pna_pre_output_metadata_t ostd)
{
    apply { }
}


control MainControlImpl(
    inout headers_t       hdr,           // from main parser
    inout main_metadata_t user_meta,     // from main parser, to "next block"
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    DirectCounter<PacketCounter_t>(PNA_CounterType_t.PACKETS_AND_BYTES) per_prefix_pkt_count;
    ActionSelector(PNA_HashAlgorithm_t.TARGET_DEFAULT, 32w1024, 32w16) as;
    action drop() {
        drop_packet();
    }

    action send(PortId_t port) {
        send_to_port(port);
	per_prefix_pkt_count.count(1024);
    }

    table ipv4_host {
        key = {
            hdr.ipv4.dstAddr : exact;
            user_meta.data : selector;
        }

        actions = {
            drop;
            send;
        }

        const default_action = drop;
        size = IPV4_HOST_TABLE_MAX_ENTRIES;
        // Direct counter and implementation property cannot co-exist
	pna_direct_counter = per_prefix_pkt_count;
        pna_implementation = as;
    }

    apply {
        ipv4_host.apply();
    }
}

control MainDeparserImpl(
    packet_out pkt,
    in    headers_t hdr,
    in    main_metadata_t user_meta,
    in    pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

