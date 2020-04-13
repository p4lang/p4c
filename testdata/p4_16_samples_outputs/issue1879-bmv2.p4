#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

const bit<6> TYPE_ADDR_IPV4 = 0x1;
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
    bit<8>     headerLen;
    bit<8>     hLeft;
    bit<9>     addrLen;
    bit<8>     currPos;
    currenti_t currenti;
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

parser SubParser(packet_in packet, out prot_i_t inf, inout metadata meta, in bool currentISelected, in bit<8> currI) {
    state start {
        packet.extract(inf);
        bool currentISelected2 = currI == meta.currPos;
        meta.currenti.upDirection = meta.currenti.upDirection + (bit<1>)currentISelected2 * inf.upDirection;
        meta.hLeft = inf.segLen;
        meta.currPos = meta.currPos + 1;
        transition accept;
    }
}

parser PROTParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    SubParser() subParser;
    state start {
        packet.extract(hdr.preamble);
        packet.extract(hdr.prot_common);
        packet.extract(hdr.prot_addr_common);
        meta.headerLen = hdr.prot_common.hdrLen;
        transition parse_prot_host_addr_dst_ipv4;
    }
    state parse_prot_host_addr_dst_ipv4 {
        packet.extract(hdr.prot_host_addr_dst.ipv4);
        meta.addrLen = 32;
        transition parse_prot_host_addr_src_select;
    }
    state parse_prot_host_addr_src_select {
        transition select(hdr.prot_common.srcType) {
            TYPE_ADDR_IPV4: parse_prot_host_addr_src_ipv4;
        }
    }
    state parse_prot_host_addr_src_ipv4 {
        packet.extract(hdr.prot_host_addr_src.ipv4);
        meta.addrLen = meta.addrLen + 32;
        transition parse_prot_addr_padding;
    }
    state parse_prot_addr_padding {
        bit<9> paddingLen = (64 - meta.addrLen % 64) % 64;
        packet.extract(hdr.prot_host_addr_padding, (bit<32>)paddingLen);
        meta.addrLen = meta.addrLen + paddingLen;
        transition parse_prot_inf_0;
    }
    state parse_prot_inf_0 {
        meta.currPos = (bit<8>)(1 + 2 + meta.addrLen / 64);
        bool currentISelected = hdr.prot_common.curri == meta.currPos;
        subParser.apply(packet, hdr.prot_inf_0, meta, currentISelected, hdr.prot_common.curri);
        transition parse_prot_h_0_pre;
    }
    state parse_prot_h_0_pre {
        transition select(meta.hLeft) {
            0: parse_prot_1;
            default: parse_prot_h_0;
        }
    }
    state parse_prot_h_0 {
        packet.extract(hdr.prot_h_0.next);
        meta.hLeft = meta.hLeft - 1;
        meta.currPos = meta.currPos + 1;
        transition parse_prot_h_0_pre;
    }
    state parse_prot_1 {
        bit<8> hdrLeft = meta.headerLen - meta.currPos;
        transition select(hdrLeft) {
            0: accept;
            default: parse_prot_inf_1;
        }
    }
    state parse_prot_inf_1 {
        bool currentISelected = meta.currPos == hdr.prot_common.curri;
        subParser.apply(packet, hdr.prot_inf_1, meta, currentISelected, hdr.prot_common.curri);
        transition accept;
    }
}

control PROTVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control PROTIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (meta.currenti.upDirection == 0) {
            mark_to_drop(standard_metadata);
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
        packet.emit(hdr);
    }
}

V1Switch(PROTParser(), PROTVerifyChecksum(), PROTIngress(), PROTEgress(), PROTComputeChecksum(), PROTDeparser()) main;

