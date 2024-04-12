#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

typedef bit<48> EthernetAddress;
struct user_meta_t {
    bit<16> data;
    bit<16> data1;
}

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

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout user_meta_t b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
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

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout user_meta_t b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("MyIC.a1") action a1(@name("param") bit<48> param) {
        hdr.ethernet.dstAddr = param;
    }
    @name("MyIC.a2") action a2(@name("param") bit<16> param_3) {
        hdr.ethernet.etherType = param_3;
    }
    @name("MyIC.a2") action a2_1(@name("param") bit<16> param_4) {
        hdr.ethernet.etherType = param_4;
    }
    @name("MyIC.a3") action a3(@name("param") bit<48> param_5) {
        hdr.ethernet.srcAddr = param_5;
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
            b.data              : exact @name("b.data");
            b.data1             : lpm @name("b.data1");
        }
        actions = {
            NoAction_1();
            a1();
            a2();
        }
        default_action = NoAction_1();
    }
    @name("MyIC.foo") table foo_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr");
            b.data              : exact @name("b.data");
            b.data1             : lpm @name("b.data1");
        }
        actions = {
            NoAction_2();
            a3();
            a2_1();
        }
        default_action = NoAction_2();
    }
    @name("MyIC.bar") table bar_0 {
        actions = {
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        if (tbl_0.apply().hit) {
            foo_0.apply();
        }
        if (tbl_0.apply().miss) {
            ;
        } else {
            foo_0.apply();
        }
        if (tbl_0.apply().hit) {
            ;
        } else {
            bar_0.apply();
        }
        if (tbl_0.apply().miss) {
            bar_0.apply();
        }
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_t e, in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, user_meta_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, user_meta_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
