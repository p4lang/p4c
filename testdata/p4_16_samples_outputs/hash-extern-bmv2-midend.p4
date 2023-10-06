error {
    UnhandledIPv4Options,
    BadIPv4HeaderChecksum
}
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

struct metadata_t {
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

struct tuple_0 {
    bit<12> f0;
}

control ingress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = istd.ingress_port;
    }
    @name("ingress.h") Hash<bit<16>>(PSA_HashAlgorithm_t.CRC16) h_0;
    @hidden action hashexternbmv2l87() {
        hdr.ethernet.etherType = 16w7;
    }
    @hidden action hashexternbmv2l85() {
        hdr.ipv4.hdrChecksum = h_0.get_hash<tuple_0>((tuple_0){f0 = 12w0x456});
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    @hidden table tbl_hashexternbmv2l85 {
        actions = {
            hashexternbmv2l85();
        }
        const default_action = hashexternbmv2l85();
    }
    @hidden table tbl_hashexternbmv2l87 {
        actions = {
            hashexternbmv2l87();
        }
        const default_action = hashexternbmv2l87();
    }
    apply {
        tbl_send_to_port.apply();
        tbl_hashexternbmv2l85.apply();
        if (hdr.ipv4.hdrChecksum == 16w0xfe82) {
            tbl_hashexternbmv2l87.apply();
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

struct tuple_1 {
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

control egress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @name("egress.h") Hash<bit<16>>(PSA_HashAlgorithm_t.CRC16) h_1;
    @hidden action hashexternbmv2l114() {
        hdr.ipv4.hdrChecksum = h_1.get_hash<bit<4>, tuple_1>(4w0x6, (tuple_1){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, 4w0xa);
    }
    @hidden table tbl_hashexternbmv2l114 {
        actions = {
            hashexternbmv2l114();
        }
        const default_action = hashexternbmv2l114();
    }
    apply {
        tbl_hashexternbmv2l114.apply();
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action hashexternbmv2l137() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_hashexternbmv2l137 {
        actions = {
            hashexternbmv2l137();
        }
        const default_action = hashexternbmv2l137();
    }
    apply {
        tbl_hashexternbmv2l137.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action hashexternbmv2l152() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_hashexternbmv2l152 {
        actions = {
            hashexternbmv2l152();
        }
        const default_action = hashexternbmv2l152();
    }
    apply {
        tbl_hashexternbmv2l152.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
