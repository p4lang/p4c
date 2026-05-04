#include <core.p4>
#include <bmv2/psa.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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
    @name("ingress.tmp") bit<16> tmp;
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = (bit<32>)hdr.ethernet.dstAddr;
    }
    @hidden action psaparsererrortestbmv2l78() {
        tmp = 16w1;
    }
    @hidden action psaparsererrortestbmv2l80() {
        tmp = 16w2;
    }
    @hidden action psaparsererrortestbmv2l82() {
        tmp = 16w3;
    }
    @hidden action psaparsererrortestbmv2l84() {
        tmp = 16w4;
    }
    @hidden action psaparsererrortestbmv2l86() {
        tmp = 16w5;
    }
    @hidden action psaparsererrortestbmv2l88() {
        tmp = 16w6;
    }
    @hidden action psaparsererrortestbmv2l90() {
        tmp = 16w7;
    }
    @hidden action psaparsererrortestbmv2l76() {
        tmp = 16w8;
    }
    @hidden action psaparsererrortestbmv2l140() {
        hdr.ethernet.dstAddr[47:32] = tmp;
        hdr.ethernet.dstAddr[31:0] = (bit<32>)(bit<64>)istd.ingress_timestamp;
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l76 {
        actions = {
            psaparsererrortestbmv2l76();
        }
        const default_action = psaparsererrortestbmv2l76();
    }
    @hidden table tbl_psaparsererrortestbmv2l78 {
        actions = {
            psaparsererrortestbmv2l78();
        }
        const default_action = psaparsererrortestbmv2l78();
    }
    @hidden table tbl_psaparsererrortestbmv2l80 {
        actions = {
            psaparsererrortestbmv2l80();
        }
        const default_action = psaparsererrortestbmv2l80();
    }
    @hidden table tbl_psaparsererrortestbmv2l82 {
        actions = {
            psaparsererrortestbmv2l82();
        }
        const default_action = psaparsererrortestbmv2l82();
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
    @hidden table tbl_psaparsererrortestbmv2l140 {
        actions = {
            psaparsererrortestbmv2l140();
        }
        const default_action = psaparsererrortestbmv2l140();
    }
    apply {
        tbl_send_to_port.apply();
        tbl_psaparsererrortestbmv2l76.apply();
        if (istd.parser_error == error.NoError) {
            tbl_psaparsererrortestbmv2l78.apply();
        } else if (istd.parser_error == error.PacketTooShort) {
            tbl_psaparsererrortestbmv2l80.apply();
        } else if (istd.parser_error == error.NoMatch) {
            tbl_psaparsererrortestbmv2l82.apply();
        } else if (istd.parser_error == error.StackOutOfBounds) {
            tbl_psaparsererrortestbmv2l84.apply();
        } else if (istd.parser_error == error.HeaderTooShort) {
            tbl_psaparsererrortestbmv2l86.apply();
        } else if (istd.parser_error == error.ParserTimeout) {
            tbl_psaparsererrortestbmv2l88.apply();
        } else if (istd.parser_error == error.ParserInvalidArgument) {
            tbl_psaparsererrortestbmv2l90.apply();
        }
        tbl_psaparsererrortestbmv2l140.apply();
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
    @name("egress.tmp_0") bit<16> tmp_0;
    @hidden action psaparsererrortestbmv2l78_0() {
        tmp_0 = 16w1;
    }
    @hidden action psaparsererrortestbmv2l80_0() {
        tmp_0 = 16w2;
    }
    @hidden action psaparsererrortestbmv2l82_0() {
        tmp_0 = 16w3;
    }
    @hidden action psaparsererrortestbmv2l84_0() {
        tmp_0 = 16w4;
    }
    @hidden action psaparsererrortestbmv2l86_0() {
        tmp_0 = 16w5;
    }
    @hidden action psaparsererrortestbmv2l88_0() {
        tmp_0 = 16w6;
    }
    @hidden action psaparsererrortestbmv2l90_0() {
        tmp_0 = 16w7;
    }
    @hidden action psaparsererrortestbmv2l76_0() {
        tmp_0 = 16w8;
    }
    @hidden action psaparsererrortestbmv2l184() {
        hdr.ethernet.srcAddr[47:32] = tmp_0;
        hdr.ethernet.srcAddr[31:0] = (bit<32>)(bit<64>)istd.egress_timestamp;
    }
    @hidden table tbl_psaparsererrortestbmv2l76_0 {
        actions = {
            psaparsererrortestbmv2l76_0();
        }
        const default_action = psaparsererrortestbmv2l76_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l78_0 {
        actions = {
            psaparsererrortestbmv2l78_0();
        }
        const default_action = psaparsererrortestbmv2l78_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l80_0 {
        actions = {
            psaparsererrortestbmv2l80_0();
        }
        const default_action = psaparsererrortestbmv2l80_0();
    }
    @hidden table tbl_psaparsererrortestbmv2l82_0 {
        actions = {
            psaparsererrortestbmv2l82_0();
        }
        const default_action = psaparsererrortestbmv2l82_0();
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
    @hidden table tbl_psaparsererrortestbmv2l184 {
        actions = {
            psaparsererrortestbmv2l184();
        }
        const default_action = psaparsererrortestbmv2l184();
    }
    apply {
        tbl_psaparsererrortestbmv2l76_0.apply();
        if (istd.parser_error == error.NoError) {
            tbl_psaparsererrortestbmv2l78_0.apply();
        } else if (istd.parser_error == error.PacketTooShort) {
            tbl_psaparsererrortestbmv2l80_0.apply();
        } else if (istd.parser_error == error.NoMatch) {
            tbl_psaparsererrortestbmv2l82_0.apply();
        } else if (istd.parser_error == error.StackOutOfBounds) {
            tbl_psaparsererrortestbmv2l84_0.apply();
        } else if (istd.parser_error == error.HeaderTooShort) {
            tbl_psaparsererrortestbmv2l86_0.apply();
        } else if (istd.parser_error == error.ParserTimeout) {
            tbl_psaparsererrortestbmv2l88_0.apply();
        } else if (istd.parser_error == error.ParserInvalidArgument) {
            tbl_psaparsererrortestbmv2l90_0.apply();
        }
        tbl_psaparsererrortestbmv2l184.apply();
    }
}

control IngressDeparserImpl(packet_out pkt, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaparsererrortestbmv2l197() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
        pkt.emit<mpls_t>(hdr.mpls);
    }
    @hidden table tbl_psaparsererrortestbmv2l197 {
        actions = {
            psaparsererrortestbmv2l197();
        }
        const default_action = psaparsererrortestbmv2l197();
    }
    apply {
        tbl_psaparsererrortestbmv2l197.apply();
    }
}

control EgressDeparserImpl(packet_out pkt, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaparsererrortestbmv2l212() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv6_t>(hdr.ipv6);
        pkt.emit<mpls_t>(hdr.mpls);
    }
    @hidden table tbl_psaparsererrortestbmv2l212 {
        actions = {
            psaparsererrortestbmv2l212();
        }
        const default_action = psaparsererrortestbmv2l212();
    }
    apply {
        tbl_psaparsererrortestbmv2l212.apply();
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
