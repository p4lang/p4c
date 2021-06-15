#include <core.p4>
#include <psa.p4>

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

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

header mpls_t {
    bit<20> label;
    bit<3>  tc;
    bit<1>  stack;
    bit<8>  ttl;
}

struct empty_metadata_t {
}

struct fwd_metadata_t {
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    ipv6_t     ipv6;
    mpls_t     mpls;
}

parser IngressParserImpl(packet_in pkt, out headers hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x8847: parse_mpls;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    state parse_mpls {
        pkt.extract<mpls_t>(hdr.mpls);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarnUnused @name(".send_to_port") action send_to_port() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = (PortIdUint_t)hdr.ethernet.dstAddr;
    }
    @hidden action psaparsererrortestbmv2l86() {
        hdr.ethernet.dstAddr[47:32] = 16w1;
    }
    @hidden action psaparsererrortestbmv2l88() {
        hdr.ethernet.dstAddr[47:32] = 16w2;
    }
    @hidden action psaparsererrortestbmv2l90() {
        hdr.ethernet.dstAddr[47:32] = 16w3;
    }
    @hidden action psaparsererrortestbmv2l92() {
        hdr.ethernet.dstAddr[47:32] = 16w4;
    }
    @hidden action psaparsererrortestbmv2l94() {
        hdr.ethernet.dstAddr[47:32] = 16w5;
    }
    @hidden action psaparsererrortestbmv2l96() {
        hdr.ethernet.dstAddr[47:32] = 16w6;
    }
    @hidden action psaparsererrortestbmv2l98() {
        hdr.ethernet.dstAddr[47:32] = 16w7;
    }
    @hidden action psaparsererrortestbmv2l84() {
        hdr.ethernet.dstAddr[47:32] = 16w8;
    }
    @hidden action psaparsererrortestbmv2l148() {
        hdr.ethernet.dstAddr[31:0] = (bit<32>)(TimestampUint_t)istd.ingress_timestamp;
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port();
        }
        const default_action = send_to_port();
    }
    @hidden table tbl_psaparsererrortestbmv2l84 {
        actions = {
            psaparsererrortestbmv2l84();
        }
        const default_action = psaparsererrortestbmv2l84();
    }
    @hidden table tbl_psaparsererrortestbmv2l86 {
        actions = {
            psaparsererrortestbmv2l86();
        }
        const default_action = psaparsererrortestbmv2l86();
    }
    @hidden table tbl_psaparsererrortestbmv2l88 {
        actions = {
            psaparsererrortestbmv2l88();
        }
        const default_action = psaparsererrortestbmv2l88();
    }
    @hidden table tbl_psaparsererrortestbmv2l90 {
        actions = {
            psaparsererrortestbmv2l90();
        }
        const default_action = psaparsererrortestbmv2l90();
    }
    @hidden table tbl_psaparsererrortestbmv2l92 {
        actions = {
            psaparsererrortestbmv2l92();
        }
        const default_action = psaparsererrortestbmv2l92();
    }
    @hidden table tbl_psaparsererrortestbmv2l94 {
        actions = {
            psaparsererrortestbmv2l94();
        }
        const default_action = psaparsererrortestbmv2l94();
    }
    @hidden table tbl_psaparsererrortestbmv2l96 {
        actions = {
            psaparsererrortestbmv2l96();
        }
        const default_action = psaparsererrortestbmv2l96();
    }
    @hidden table tbl_psaparsererrortestbmv2l98 {
        actions = {
            psaparsererrortestbmv2l98();
        }
        const default_action = psaparsererrortestbmv2l98();
    }
    @hidden table tbl_psaparsererrortestbmv2l148 {
        actions = {
            psaparsererrortestbmv2l148();
        }
        const default_action = psaparsererrortestbmv2l148();
    }
    apply {
        tbl_send_to_port.apply();
        tbl_psaparsererrortestbmv2l84.apply();
        if (istd.parser_error == error.NoError) {
            tbl_psaparsererrortestbmv2l86.apply();
        } else if (istd.parser_error == error.PacketTooShort) {
            tbl_psaparsererrortestbmv2l88.apply();
        } else if (istd.parser_error == error.NoMatch) {
            tbl_psaparsererrortestbmv2l90.apply();
        } else if (istd.parser_error == error.StackOutOfBounds) {
            tbl_psaparsererrortestbmv2l92.apply();
        } else if (istd.parser_error == error.HeaderTooShort) {
            tbl_psaparsererrortestbmv2l94.apply();
        } else if (istd.parser_error == error.ParserTimeout) {
            tbl_psaparsererrortestbmv2l96.apply();
        } else if (istd.parser_error == error.ParserInvalidArgument) {
            tbl_psaparsererrortestbmv2l98.apply();
        }
        tbl_psaparsererrortestbmv2l148.apply();
    }
}

parser EgressParserImpl(packet_in pkt, out headers hdr, inout metadata user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x86dd: parse_ipv6;
            16w0x8847: parse_mpls;
            default: accept;
        }
    }
    state parse_ipv6 {
        pkt.extract<ipv6_t>(hdr.ipv6);
        transition accept;
    }
    state parse_mpls {
        pkt.extract<mpls_t>(hdr.mpls);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @hidden action psaparsererrortestbmv2l86_0() {
        hdr.ethernet.srcAddr[47:32] = 16w1;
    }
    @hidden action psaparsererrortestbmv2l88_0() {
        hdr.ethernet.srcAddr[47:32] = 16w2;
    }
    @hidden action psaparsererrortestbmv2l90_0() {
        hdr.ethernet.srcAddr[47:32] = 16w3;
    }
    @hidden action psaparsererrortestbmv2l92_0() {
        hdr.ethernet.srcAddr[47:32] = 16w4;
    }
    @hidden action psaparsererrortestbmv2l94_0() {
        hdr.ethernet.srcAddr[47:32] = 16w5;
    }
    @hidden action psaparsererrortestbmv2l96_0() {
        hdr.ethernet.srcAddr[47:32] = 16w6;
    }
    @hidden action psaparsererrortestbmv2l98_0() {
        hdr.ethernet.srcAddr[47:32] = 16w7;
    }
    @hidden action psaparsererrortestbmv2l84_0() {
        hdr.ethernet.srcAddr[47:32] = 16w8;
    }
    @hidden action psaparsererrortestbmv2l192() {
        hdr.ethernet.srcAddr[31:0] = (bit<32>)(TimestampUint_t)istd.egress_timestamp;
    }
    @hidden table tbl_psaparsererrortestbmv2l84_0 {
        actions = {
            psaparsererrortestbmv2l84_0();
        }
        const default_action = psaparsererrortestbmv2l84_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l86_0 {
        actions = {
            psaparsererrortestbmv2l86_0();
        }
        const default_action = psaparsererrortestbmv2l86_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l88_0 {
        actions = {
            psaparsererrortestbmv2l88_0();
        }
        const default_action = psaparsererrortestbmv2l88_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l90_0 {
        actions = {
            psaparsererrortestbmv2l90_0();
        }
        const default_action = psaparsererrortestbmv2l90_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l92_0 {
        actions = {
            psaparsererrortestbmv2l92_0();
        }
        const default_action = psaparsererrortestbmv2l92_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l94_0 {
        actions = {
            psaparsererrortestbmv2l94_0();
        }
        const default_action = psaparsererrortestbmv2l94_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l96_0 {
        actions = {
            psaparsererrortestbmv2l96_0();
        }
        const default_action = psaparsererrortestbmv2l96_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l98_0 {
        actions = {
            psaparsererrortestbmv2l98_0();
        }
        const default_action = psaparsererrortestbmv2l98_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l192 {
        actions = {
            psaparsererrortestbmv2l192();
        }
        const default_action = psaparsererrortestbmv2l192();
    }
    apply {
        tbl_psaparsererrortestbmv2l84_0.apply();
        if (istd.parser_error == error.NoError) {
            tbl_psaparsererrortestbmv2l86_0.apply();
        } else if (istd.parser_error == error.PacketTooShort) {
            tbl_psaparsererrortestbmv2l88_0.apply();
        } else if (istd.parser_error == error.NoMatch) {
            tbl_psaparsererrortestbmv2l90_0.apply();
        } else if (istd.parser_error == error.StackOutOfBounds) {
            tbl_psaparsererrortestbmv2l92_0.apply();
        } else if (istd.parser_error == error.HeaderTooShort) {
            tbl_psaparsererrortestbmv2l94_0.apply();
        } else if (istd.parser_error == error.ParserTimeout) {
            tbl_psaparsererrortestbmv2l96_0.apply();
        } else if (istd.parser_error == error.ParserInvalidArgument) {
            tbl_psaparsererrortestbmv2l98_0.apply();
        }
        tbl_psaparsererrortestbmv2l192.apply();
    }
}

control IngressDeparserImpl(packet_out pkt, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaparsererrortestbmv2l205() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
        pkt.emit<mpls_t>(hdr.mpls);
    }
    @hidden table tbl_psaparsererrortestbmv2l205 {
        actions = {
            psaparsererrortestbmv2l205();
        }
        const default_action = psaparsererrortestbmv2l205();
    }
    apply {
        tbl_psaparsererrortestbmv2l205.apply();
    }
}

control EgressDeparserImpl(packet_out pkt, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaparsererrortestbmv2l220() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv6_t>(hdr.ipv6);
        pkt.emit<mpls_t>(hdr.mpls);
    }
    @hidden table tbl_psaparsererrortestbmv2l220 {
        actions = {
            psaparsererrortestbmv2l220();
        }
        const default_action = psaparsererrortestbmv2l220();
    }
    apply {
        tbl_psaparsererrortestbmv2l220.apply();
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

