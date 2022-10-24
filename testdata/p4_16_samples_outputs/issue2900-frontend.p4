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
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.key_0") bit<32> key_0;
    @name("MainControlImpl.key_1") bit<32> key_1;
    @name("MainControlImpl.key_2") bit<8> key_2;
    @name("MainControlImpl.tmp") bool tmp;
    @name("MainControlImpl.istd_0") pna_main_input_metadata_t istd_1;
    @name("MainControlImpl.retval") bool retval;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.clb_pinned_flows") table clb_pinned_flows_0 {
        key = {
            key_0: exact @name("ipv4_addr_0");
            key_1: exact @name("ipv4_addr_1");
            key_2: exact @name("hdr.ipv4.protocol");
        }
        actions = {
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    apply {
        istd_1 = istd;
        retval = istd_1.direction == PNA_Direction_t.HOST_TO_NET;
        tmp = retval;
        if (tmp) {
            key_0 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr);
            key_1 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr);
            key_2 = hdr.ipv4.protocol;
            clb_pinned_flows_0.apply();
        } else {
            key_0 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr);
            key_1 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr);
            key_2 = hdr.ipv4.protocol;
            clb_pinned_flows_0.apply();
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
