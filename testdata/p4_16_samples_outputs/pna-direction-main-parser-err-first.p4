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

struct main_metadata_t {
    bit<32> tmpDir;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
        if (PNA_Direction_t.NET_TO_HOST == istd.direction) {
            meta.tmpDir = hdr.ipv4.srcAddr;
        } else {
            meta.tmpDir = hdr.ipv4.dstAddr;
        }
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    @name("tmpDir") bit<32> tmpDir_0;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition select(PNA_Direction_t.NET_TO_HOST == istd.direction) {
            true: parse_ipv4_true;
            false: parse_ipv4_false;
        }
    }
    state parse_ipv4_true {
        tmpDir_0 = hdr.ipv4.srcAddr;
        transition parse_ipv4_join;
    }
    state parse_ipv4_false {
        tmpDir_0 = hdr.ipv4.dstAddr;
        transition parse_ipv4_join;
    }
    state parse_ipv4_join {
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("tmpDir") bit<32> tmpDir_1;
    @name("next_hop") action next_hop_0(PortId_t vport) {
        send_to_port(vport);
    }
    @name("default_route_drop") action default_route_drop_0() {
        drop_packet();
    }
    @name("ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            tmpDir_1: lpm @name("ipv4_addr");
        }
        actions = {
            next_hop_0();
            default_route_drop_0();
        }
        const default_action = default_route_drop_0();
    }
    apply {
        if (PNA_Direction_t.NET_TO_HOST == istd.direction) {
            tmpDir_1 = hdr.ipv4.srcAddr;
        } else {
            tmpDir_1 = hdr.ipv4.dstAddr;
        }
        if (hdr.ipv4.isValid()) {
            ipv4_da_lpm_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
