#include <core.p4>
#include "pna.p4"

const MirrorSlotId_t MIRROR_SLOT_ID = (MirrorSlotId_t) 3;

const MirrorSessionId_t MIRROR_SESSION1 = (MirrorSessionId_t) 58;
const MirrorSessionId_t MIRROR_SESSION2 = (MirrorSessionId_t) 62;
const MirrorSessionId_t MIRROR_SESSION3 = (MirrorSessionId_t) 6;

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

struct headers_t {
    ethernet_t ethernet;
    ipv4_t ipv4;
}

struct main_metadata_t {
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
        transition select (hdr.ipv4.protocol) {
            default: accept;
        }
    }
}

control MainControlImpl(
    inout headers_t  hdr,
    inout main_metadata_t meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    MirrorSessionId_t mirror_session;
    MirrorSlotId_t mirror_slot; 
    action send_with_mirror (PortId_t vport) {
	send_to_port(vport);
	mirror_packet(mirror_slot, mirror_session);
    }

    action drop_with_mirror() {
	drop_packet();
	mirror_packet(mirror_slot, mirror_session);
    }

    table flowTable {
        key = {
            hdr.ipv4.srcAddr : exact;
            hdr.ipv4.dstAddr : exact;
            hdr.ipv4.protocol : exact;
        }
        actions = {
            send_with_mirror;
            drop_with_mirror;
            NoAction;
        }
        const default_action = NoAction();
    }

    apply {
                mirror_slot = MIRROR_SLOT_ID;
                switch (flowTable.apply().action_run){
                    send_with_mirror: { mirror_session = MIRROR_SESSION1;}
                    drop_with_mirror: { mirror_session = MIRROR_SESSION2;}
                    default: {mirror_session = MIRROR_SESSION3;}
                }
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

PNA_NIC(
    MainParserImpl(),
    PreControlImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;

