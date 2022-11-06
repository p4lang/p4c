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
    bit<80> newfield;
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

enum bit<8> Proto {
    proto1 = 8w0x1,
    proto2 = 8w0x2
}

struct empty_metadata_t {
}

struct metadata {
    bit<16> data;
    bit<48> key1;
    bit<48> key2;
    bit<48> key3;
    bit<48> key4;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

parser IngressParserImpl(packet_in buffer, out headers hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800 &&& 16w0xf00: parse_ipv4;
            16w0xd00: parse_tcp;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w4 .. 8w7: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        buffer.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("ingress.execute") action execute_1(@name("x") bit<48> x) {
        user_meta.data = 16w1;
    }
    @name("ingress.execute") action execute_2(@name("x") bit<48> x_1) {
        user_meta.data = 16w1;
    }
    @name("ingress.tbl") table tbl_0 {
        key = {
            hdr.ethernet.isValid(): exact @name("hdr.ethernet.$valid$");
            hdr.ethernet.dstAddr  : exact @name("hdr.ethernet.dstAddr");
            hdr.ethernet.srcAddr  : exact @name("hdr.ethernet.srcAddr");
            hdr.ipv4.protocol     : exact @name("hdr.ipv4.protocol");
            user_meta.key1        : ternary @name("user_meta.key1");
            user_meta.key2        : range @name("user_meta.key2");
            user_meta.key4        : optional @name("user_meta.key4");
        }
        actions = {
            NoAction_1();
            execute_1();
        }
        const entries = {
                        (true, 48w1, 48w2, Proto.proto1, 48w2 &&& 48w3, 48w2 .. 48w4, 48w10) : execute_1(48w1);
                        (true, 48w1, 48w2, Proto.proto1, 48w2 &&& 48w3, 48w2 .. 48w5, 48w10) : execute_1(48w1);
                        (true, 48w3, 48w3, Proto.proto1, 48w2 &&& 48w3, 48w2 .. 48w6, 48w10) : execute_1(48w1);
        }
        default_action = NoAction_1();
    }
    @name("ingress.tbl1") table tbl1_0 {
        key = {
            hdr.ethernet.isValid(): exact @name("hdr.ethernet.$valid$");
            hdr.ethernet.dstAddr  : exact @name("hdr.ethernet.dstAddr");
            hdr.ethernet.srcAddr  : exact @name("hdr.ethernet.srcAddr");
            user_meta.key3        : lpm @name("user_meta.key3");
        }
        actions = {
            NoAction_2();
            execute_2();
        }
        const entries = {
                        (true, 48w1, 48w2, 48w10) : execute_2(48w1);
                        (true, 48w1, 48w2, 48w11) : execute_2(48w1);
                        (true, 48w3, 48w3, 48w12) : execute_2(48w1);
        }
        default_action = NoAction_2();
    }
    apply {
        tbl_0.apply();
        tbl1_0.apply();
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
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
