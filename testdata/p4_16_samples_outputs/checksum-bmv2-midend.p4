error {
    UnhandledIPv4Options,
    BadIPv4HeaderChecksum
}
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

struct empty_metadata_t {
}

struct fwd_metadata_t {
}

struct metadata_t {
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct tuple_0 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<3>  f5;
    bit<13> f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<32> f9;
    bit<32> f10;
}

parser IngressParserImpl(packet_in buffer, out headers_t parsed_hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    @name("IngressParserImpl.ck") Checksum<bit<16>>(PSA_HashAlgorithm_t.CRC16) ck_0;
    state start {
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract<ipv4_t>(parsed_hdr.ipv4);
        ck_0.clear();
        ck_0.update<tuple_0>({ parsed_hdr.ipv4.version, parsed_hdr.ipv4.ihl, parsed_hdr.ipv4.diffserv, parsed_hdr.ipv4.totalLen, parsed_hdr.ipv4.identification, parsed_hdr.ipv4.flags, parsed_hdr.ipv4.fragOffset, parsed_hdr.ipv4.ttl, parsed_hdr.ipv4.protocol, parsed_hdr.ipv4.srcAddr, parsed_hdr.ipv4.dstAddr });
        verify(parsed_hdr.ipv4.hdrChecksum == ck_0.get(), error.BadIPv4HeaderChecksum);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarnUnused @name(".send_to_port") action send_to_port() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = istd.ingress_port;
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port();
        }
        const default_action = send_to_port();
    }
    apply {
        tbl_send_to_port.apply();
    }
}

parser EgressParserImpl(packet_in buffer, out headers_t parsed_hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        buffer.extract<ipv4_t>(parsed_hdr.ipv4);
        transition accept;
    }
}

control egress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @hidden action checksumbmv2l121() {
        hdr.ipv4.version = 4w5;
    }
    @hidden table tbl_checksumbmv2l121 {
        actions = {
            checksumbmv2l121();
        }
        const default_action = checksumbmv2l121();
    }
    apply {
        tbl_checksumbmv2l121.apply();
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action checksumbmv2l136() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_checksumbmv2l136 {
        actions = {
            checksumbmv2l136();
        }
        const default_action = checksumbmv2l136();
    }
    apply {
        tbl_checksumbmv2l136.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t parsed_hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @name("EgressDeparserImpl.ck") Checksum<bit<16>>(PSA_HashAlgorithm_t.CRC16) ck_1;
    @hidden action checksumbmv2l151() {
        ck_1.clear();
        ck_1.update<tuple_0>({ parsed_hdr.ipv4.version, parsed_hdr.ipv4.ihl, parsed_hdr.ipv4.diffserv, parsed_hdr.ipv4.totalLen, parsed_hdr.ipv4.identification, parsed_hdr.ipv4.flags, parsed_hdr.ipv4.fragOffset, parsed_hdr.ipv4.ttl, parsed_hdr.ipv4.protocol, parsed_hdr.ipv4.srcAddr, parsed_hdr.ipv4.dstAddr });
        parsed_hdr.ipv4.hdrChecksum = ck_1.get();
        buffer.emit<ethernet_t>(parsed_hdr.ethernet);
        buffer.emit<ipv4_t>(parsed_hdr.ipv4);
    }
    @hidden table tbl_checksumbmv2l151 {
        actions = {
            checksumbmv2l151();
        }
        const default_action = checksumbmv2l151();
    }
    apply {
        tbl_checksumbmv2l151.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

