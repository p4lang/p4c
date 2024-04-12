#include <core.p4>
#include <bmv2/psa.p4>

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
    fwd_metadata_t fwd_metadata;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    ipv6_t     ipv6;
    mpls_t     mpls;
}

control parser_error_to_int(in error err, out bit<16> ret) {
    apply {
        ret = 16w8;
        if (err == error.NoError) {
            ret = 16w1;
        } else if (err == error.PacketTooShort) {
            ret = 16w2;
        } else if (err == error.NoMatch) {
            ret = 16w3;
        } else if (err == error.StackOutOfBounds) {
            ret = 16w4;
        } else if (err == error.HeaderTooShort) {
            ret = 16w5;
        } else if (err == error.ParserTimeout) {
            ret = 16w6;
        } else if (err == error.ParserInvalidArgument) {
            ret = 16w7;
        }
    }
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
    @name("parser_error_to_int") parser_error_to_int() parser_error_to_int_inst;
    apply {
        send_to_port(ostd, (PortId_t)(PortIdUint_t)hdr.ethernet.dstAddr);
        parser_error_to_int_inst.apply(istd.parser_error, hdr.ethernet.dstAddr[47:32]);
        hdr.ethernet.dstAddr[31:0] = (bit<32>)(TimestampUint_t)istd.ingress_timestamp;
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
    @name("parser_error_to_int") parser_error_to_int() parser_error_to_int_inst_0;
    apply {
        parser_error_to_int_inst_0.apply(istd.parser_error, hdr.ethernet.srcAddr[47:32]);
        hdr.ethernet.srcAddr[31:0] = (bit<32>)(TimestampUint_t)istd.egress_timestamp;
    }
}

control IngressDeparserImpl(packet_out pkt, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
        pkt.emit<mpls_t>(hdr.mpls);
    }
}

control EgressDeparserImpl(packet_out pkt, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv6_t>(hdr.ipv6);
        pkt.emit<mpls_t>(hdr.mpls);
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
