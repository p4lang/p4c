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
    bit<32> old_srcAddr;
}

struct metadata {
    bit<32> _fwd_metadata_old_srcAddr0;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
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
        transition select(parsed_hdr.ipv4.protocol) {
            8w6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        buffer.extract<tcp_t>(parsed_hdr.tcp);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.drop") action drop_1() {
        @noWarnUnused {
            ostd.drop = true;
        }
    }
    @name("ingress.forward") action forward(@name("port") PortId_t port, @name("srcAddr") bit<32> srcAddr_1) {
        user_meta._fwd_metadata_old_srcAddr0 = hdr.ipv4.srcAddr;
        hdr.ipv4.srcAddr = srcAddr_1;
        @noWarnUnused {
            ostd.drop = false;
            ostd.multicast_group = 32w0;
            ostd.egress_port = port;
        }
    }
    @name("ingress.route") table route_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            forward();
            drop_1();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            route_0.apply();
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

struct tuple_1 {
    bit<16> f0;
}

struct tuple_2 {
    bit<32> f0;
}

control IngressDeparserImpl(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata user_meta, in psa_ingress_output_metadata_t istd) {
    @name("IngressDeparserImpl.ck") InternetChecksum() ck_0;
    @hidden action psaexampledpdkbytealignment_4l171() {
        ck_0.clear();
        ck_0.add<tuple_0>((tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr});
        hdr.ipv4.hdrChecksum = ck_0.get();
        ck_0.clear();
        ck_0.subtract<tuple_1>((tuple_1){f0 = hdr.tcp.checksum});
        ck_0.subtract<tuple_2>((tuple_2){f0 = user_meta._fwd_metadata_old_srcAddr0});
        ck_0.add<tuple_2>((tuple_2){f0 = hdr.ipv4.srcAddr});
        hdr.tcp.checksum = ck_0.get();
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
    @hidden table tbl_psaexampledpdkbytealignment_4l171 {
        actions = {
            psaexampledpdkbytealignment_4l171();
        }
        const default_action = psaexampledpdkbytealignment_4l171();
    }
    apply {
        tbl_psaexampledpdkbytealignment_4l171.apply();
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata user_meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaexampledpdkbytealignment_4l215() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
    @hidden table tbl_psaexampledpdkbytealignment_4l215 {
        actions = {
            psaexampledpdkbytealignment_4l215();
        }
        const default_action = psaexampledpdkbytealignment_4l215();
    }
    apply {
        tbl_psaexampledpdkbytealignment_4l215.apply();
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

