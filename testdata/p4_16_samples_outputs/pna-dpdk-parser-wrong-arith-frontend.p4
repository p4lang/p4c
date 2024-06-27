#include <core.p4>
#include <dpdk/pna.p4>

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

struct user_metadata_t {
    bit<1> ipv4_hdr_truncated;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

control PreControlImpl(in headers_t hdr, inout user_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout user_metadata_t user_meta, in pna_main_parser_input_metadata_t istd) {
    @name("MainParserImpl.ihl") bit<4> ihl_0;
    @name("MainParserImpl.tmp") ipv4_t tmp;
    state start {
        user_meta.ipv4_hdr_truncated = 1w0;
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        tmp = pkt.lookahead<ipv4_t>();
        ihl_0 = tmp.ihl;
        transition select(ihl_0) {
            4w0x0 &&& 4w0xc: parse_ipv4_ihl_too_small;
            4w0x4: parse_ipv4_ihl_too_small;
            default: parse_ipv4_ok;
        }
    }
    state parse_ipv4_ihl_too_small {
        user_meta.ipv4_hdr_truncated = 1w1;
        transition accept;
    }
    state parse_ipv4_ok {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout user_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.tmp") bit<8> tmp_0;
    apply {
        if (hdr.ethernet.isValid()) {
            tmp_0 = 8w0;
            tmp_0[0:0] = user_meta.ipv4_hdr_truncated;
            tmp_0[1:1] = (bit<1>)hdr.ipv4.isValid();
            hdr.ethernet.srcAddr[7:0] = tmp_0;
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in user_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
}

PNA_NIC<headers_t, user_metadata_t, headers_t, user_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
