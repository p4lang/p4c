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
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

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
        transition select(hdr.ipv4.protocol) {
            default: accept;
        }
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.send_with_mirror") action send_with_mirror(@name("vport") PortId_t vport) {
        send_to_port(vport);
        mirror_packet((MirrorSlotId_t)8w3, (MirrorSessionId_t)16w58);
    }
    @name("MainControlImpl.drop_with_mirror") action drop_with_mirror() {
        drop_packet();
        mirror_packet((MirrorSlotId_t)8w3, (MirrorSessionId_t)16w62);
    }
    @name("MainControlImpl.flowTable") table flowTable_0 {
        key = {
            hdr.ipv4.srcAddr : exact @name("hdr.ipv4.srcAddr");
            hdr.ipv4.dstAddr : exact @name("hdr.ipv4.dstAddr");
            hdr.ipv4.protocol: exact @name("hdr.ipv4.protocol");
        }
        actions = {
            send_with_mirror();
            drop_with_mirror();
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    apply {
        flowTable_0.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
