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

struct empty_metadata_t {
}

struct fwd_metadata_t {
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract<ipv4_t>(parsed_hdr.ipv4);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.tmp") bit<64> tmp_0;
    @name("ingress.s") bit<64> s_0;
    @name(".update_pkt_ip_byte_count") action update_pkt_ip_byte_count_0() {
        s_0 = tmp_0;
        s_0[63:48] = tmp_0[63:48] + 16w1;
        s_0[47:0] = s_0[47:0] + 48w14;
        tmp_0 = s_0;
    }
    @name("ingress.port_pkt_ip_bytes_in") Register<bit<64>, bit<32>>(32w512) port_pkt_ip_bytes_in_0;
    @hidden action psaexampleregister2bmv2l130() {
        tmp_0 = port_pkt_ip_bytes_in_0.read(istd.ingress_port);
    }
    @hidden action psaexampleregister2bmv2l132() {
        port_pkt_ip_bytes_in_0.write(istd.ingress_port, tmp_0);
    }
    @hidden action psaexampleregister2bmv2l123() {
        ostd.egress_port = 32w0;
        hdr.ipv4.setValid();
        hdr.ipv4.totalLen = 16w14;
    }
    @hidden table tbl_psaexampleregister2bmv2l123 {
        actions = {
            psaexampleregister2bmv2l123();
        }
        const default_action = psaexampleregister2bmv2l123();
    }
    @hidden table tbl_psaexampleregister2bmv2l130 {
        actions = {
            psaexampleregister2bmv2l130();
        }
        const default_action = psaexampleregister2bmv2l130();
    }
    @hidden table tbl_update_pkt_ip_byte_count {
        actions = {
            update_pkt_ip_byte_count_0();
        }
        const default_action = update_pkt_ip_byte_count_0();
    }
    @hidden table tbl_psaexampleregister2bmv2l132 {
        actions = {
            psaexampleregister2bmv2l132();
        }
        const default_action = psaexampleregister2bmv2l132();
    }
    apply {
        tbl_psaexampleregister2bmv2l123.apply();
        if (hdr.ipv4.isValid()) @atomic {
            tbl_psaexampleregister2bmv2l130.apply();
            tbl_update_pkt_ip_byte_count.apply();
            tbl_psaexampleregister2bmv2l132.apply();
        }
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaexampleregister2bmv2l164() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_psaexampleregister2bmv2l164 {
        actions = {
            psaexampleregister2bmv2l164();
        }
        const default_action = psaexampleregister2bmv2l164();
    }
    apply {
        tbl_psaexampleregister2bmv2l164.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaexampleregister2bmv2l164_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_psaexampleregister2bmv2l164_0 {
        actions = {
            psaexampleregister2bmv2l164_0();
        }
        const default_action = psaexampleregister2bmv2l164_0();
    }
    apply {
        tbl_psaexampleregister2bmv2l164_0.apply();
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
