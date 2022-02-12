#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header preamble_t {
    bit<336> data;
}

header prot_common_t {
    bit<4>  version;
    bit<6>  dstType;
    bit<6>  srcType;
    bit<16> totalLen;
    bit<8>  hdrLen;
    bit<8>  curri;
    bit<8>  currh;
    bit<8>  nextHdr;
}

header prot_addr_common_t {
    bit<128> data;
}

header prot_host_addr_ipv4_t {
    bit<32> addr;
}

header_union prot_host_addr_t {
    prot_host_addr_ipv4_t ipv4;
}

header prot_host_addr_padding_t {
    varbit<32> padding;
}

header prot_i_t {
    bit<7>  data;
    bit<1>  upDirection;
    bit<48> data2;
    bit<8>  segLen;
}

header prot_h_t {
    bit<64> data;
}

struct currenti_t {
    bit<1> upDirection;
}

struct metadata {
    bit<8> _headerLen0;
    bit<8> _hLeft1;
    bit<9> _addrLen2;
    bit<8> _currPos3;
    bit<1> _currenti_upDirection4;
}

struct headers {
    preamble_t               preamble;
    prot_common_t            prot_common;
    prot_addr_common_t       prot_addr_common;
    prot_host_addr_t         prot_host_addr_dst;
    prot_host_addr_t         prot_host_addr_src;
    prot_host_addr_padding_t prot_host_addr_padding;
    prot_i_t                 prot_inf_0;
    prot_h_t[10]             prot_h_0;
    prot_i_t                 prot_inf_1;
}

parser PROTParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("PROTParser.inf_0") prot_i_t inf_0;
    currenti_t meta_0_currenti;
}

control PROTVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control PROTIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action issue1879bmv2l206() {
        mark_to_drop(standard_metadata);
    }
    @hidden table tbl_issue1879bmv2l206 {
        actions = {
            issue1879bmv2l206();
        }
        const default_action = issue1879bmv2l206();
    }
    apply {
        if (meta._currenti_upDirection4 == 1w0) {
            tbl_issue1879bmv2l206.apply();
        }
    }
}

control PROTEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control PROTComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control PROTDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<preamble_t>(hdr.preamble);
        packet.emit<prot_common_t>(hdr.prot_common);
        packet.emit<prot_addr_common_t>(hdr.prot_addr_common);
        packet.emit<prot_host_addr_ipv4_t>(hdr.prot_host_addr_dst.ipv4);
        packet.emit<prot_host_addr_ipv4_t>(hdr.prot_host_addr_src.ipv4);
        packet.emit<prot_host_addr_padding_t>(hdr.prot_host_addr_padding);
        packet.emit<prot_i_t>(hdr.prot_inf_0);
        packet.emit<prot_h_t>(hdr.prot_h_0[0]);
        packet.emit<prot_h_t>(hdr.prot_h_0[1]);
        packet.emit<prot_h_t>(hdr.prot_h_0[2]);
        packet.emit<prot_h_t>(hdr.prot_h_0[3]);
        packet.emit<prot_h_t>(hdr.prot_h_0[4]);
        packet.emit<prot_h_t>(hdr.prot_h_0[5]);
        packet.emit<prot_h_t>(hdr.prot_h_0[6]);
        packet.emit<prot_h_t>(hdr.prot_h_0[7]);
        packet.emit<prot_h_t>(hdr.prot_h_0[8]);
        packet.emit<prot_h_t>(hdr.prot_h_0[9]);
        packet.emit<prot_i_t>(hdr.prot_inf_1);
    }
}

V1Switch<headers, metadata>(PROTParser(), PROTVerifyChecksum(), PROTIngress(), PROTEgress(), PROTComputeChecksum(), PROTDeparser()) main;

