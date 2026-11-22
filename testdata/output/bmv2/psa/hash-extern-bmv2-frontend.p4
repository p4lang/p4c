error {
    UnhandledIPv4Options,
    BadIPv4HeaderChecksum
}
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

struct empty_metadata_t {
}

struct fwd_metadata_t {
}

struct metadata_t {
    fwd_metadata_t fwd_metadata;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser IngressParserImpl(packet_in buffer, out headers_t parsed_hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
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

control ingress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.a") bit<12> a_0;
    @name("ingress.meta") psa_ingress_output_metadata_t meta_0;
    @name("ingress.egress_port") PortId_t egress_port_0;
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        meta_0 = ostd;
        egress_port_0 = istd.ingress_port;
        meta_0.drop = false;
        meta_0.multicast_group = (MulticastGroup_t)32w0;
        meta_0.egress_port = egress_port_0;
        ostd = meta_0;
    }
    @name("ingress.h") Hash<bit<16>>(PSA_HashAlgorithm_t.CRC16) h_0;
    apply {
        a_0 = 12w0x456;
        send_to_port_0();
        hdr.ipv4.hdrChecksum = h_0.get_hash<tuple<bit<12>>>({ a_0 });
        if (hdr.ipv4.hdrChecksum == 16w0xfe82) {
            hdr.ethernet.etherType = 16w7;
        }
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
    @name("egress.base") bit<4> base_0;
    @name("egress.max") bit<4> max_0;
    @name("egress.h") Hash<bit<16>>(PSA_HashAlgorithm_t.CRC16) h_1;
    apply {
        base_0 = 4w0x6;
        max_0 = 4w0xa;
        hdr.ipv4.hdrChecksum = h_1.get_hash<bit<4>, tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>(base_0, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, max_0);
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
