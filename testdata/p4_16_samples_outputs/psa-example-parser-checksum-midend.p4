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

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
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
    tcp_t      tcp;
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

parser IngressParserImpl(packet_in buffer, out headers hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    @name("IngressParserImpl.ck") InternetChecksum() ck_0;
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract<ipv4_t>(hdr.ipv4);
        verify(hdr.ipv4.ihl == 4w5, error.UnhandledIPv4Options);
        ck_0.clear();
        ck_0.add<tuple_0>((tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr});
        verify(ck_0.get() == hdr.ipv4.hdrChecksum, error.BadIPv4HeaderChecksum);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        buffer.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    bool hasExited;
    @noWarn("unused") @name(".ingress_drop") action ingress_drop_0() {
        ostd.drop = true;
    }
    @name("ingress.parser_error_counts") DirectCounter<bit<32>>(PSA_CounterType_t.PACKETS) parser_error_counts_0;
    @name("ingress.set_error_idx") action set_error_idx(@name("idx") bit<8> idx) {
        parser_error_counts_0.count();
    }
    @name("ingress.parser_error_count_and_convert") table parser_error_count_and_convert_0 {
        key = {
            istd.parser_error: exact @name("istd.parser_error");
        }
        actions = {
            set_error_idx();
        }
        default_action = set_error_idx(8w0);
        const entries = {
                        error.NoError : set_error_idx(8w1);
                        error.PacketTooShort : set_error_idx(8w2);
                        error.NoMatch : set_error_idx(8w3);
                        error.StackOutOfBounds : set_error_idx(8w4);
                        error.HeaderTooShort : set_error_idx(8w5);
                        error.ParserTimeout : set_error_idx(8w6);
                        error.BadIPv4HeaderChecksum : set_error_idx(8w7);
                        error.UnhandledIPv4Options : set_error_idx(8w8);
        }
        psa_direct_counter = parser_error_counts_0;
    }
    @hidden action psaexampleparserchecksum185() {
        hasExited = true;
    }
    @hidden action act() {
        hasExited = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_ingress_drop {
        actions = {
            ingress_drop_0();
        }
        const default_action = ingress_drop_0();
    }
    @hidden table tbl_psaexampleparserchecksum185 {
        actions = {
            psaexampleparserchecksum185();
        }
        const default_action = psaexampleparserchecksum185();
    }
    apply {
        tbl_act.apply();
        if (istd.parser_error != error.NoError) {
            parser_error_count_and_convert_0.apply();
            tbl_ingress_drop.apply();
            tbl_psaexampleparserchecksum185.apply();
        }
    }
}

parser EgressParserImpl(packet_in buffer, out headers hdr, inout metadata user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaexampleparserchecksum222() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
    @hidden table tbl_psaexampleparserchecksum222 {
        actions = {
            psaexampleparserchecksum222();
        }
        const default_action = psaexampleparserchecksum222();
    }
    apply {
        tbl_psaexampleparserchecksum222.apply();
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @name("EgressDeparserImpl.ck") InternetChecksum() ck_1;
    @hidden action psaexampleparserchecksum239() {
        ck_1.clear();
        ck_1.add<tuple_0>((tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr});
        hdr.ipv4.hdrChecksum = ck_1.get();
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
    @hidden table tbl_psaexampleparserchecksum239 {
        actions = {
            psaexampleparserchecksum239();
        }
        const default_action = psaexampleparserchecksum239();
    }
    apply {
        tbl_psaexampleparserchecksum239.apply();
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
