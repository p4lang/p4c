#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    EthernetAddress srcAddr2;
    EthernetAddress srcAddr3;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout EMPTY b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout EMPTY b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("MyIC.a1") action a1(@name("param") bit<48> param) {
        hdr.ethernet.dstAddr = param;
    }
    @name("MyIC.a1") action a1_1(@name("param") bit<48> param_2) {
        hdr.ethernet.dstAddr = param_2;
    }
    @name("MyIC.a1") action a1_2(@name("param") bit<48> param_3) {
        hdr.ethernet.dstAddr = param_3;
    }
    @name("MyIC.a2") action a2(@name("param") bit<16> param_4) {
        hdr.ethernet.etherType = param_4;
    }
    @name("MyIC.a2") action a2_1(@name("param") bit<16> param_5) {
        hdr.ethernet.etherType = param_5;
    }
    @name("MyIC.a2") action a2_2(@name("param") bit<16> param_6) {
        hdr.ethernet.etherType = param_6;
    }
    @name("MyIC.tbl_idle_timeout") table tbl_idle_timeout_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            a1();
            a2();
        }
        psa_idle_timeout = PSA_IdleTimeout_t.NOTIFY_CONTROL;
        default_action = NoAction_1();
    }
    @name("MyIC.tbl_no_idle_timeout") table tbl_no_idle_timeout_0 {
        key = {
            hdr.ethernet.srcAddr2: exact @name("hdr.ethernet.srcAddr2");
        }
        actions = {
            NoAction_2();
            a1_1();
            a2_1();
        }
        psa_idle_timeout = PSA_IdleTimeout_t.NO_TIMEOUT;
        default_action = NoAction_2();
    }
    @name("MyIC.tbl_no_idle_timeout_prop") table tbl_no_idle_timeout_prop_0 {
        key = {
            hdr.ethernet.srcAddr2: exact @name("hdr.ethernet.srcAddr2");
        }
        actions = {
            NoAction_3();
            a1_2();
            a2_2();
        }
        default_action = NoAction_3();
    }
    apply {
        tbl_idle_timeout_0.apply();
        tbl_no_idle_timeout_0.apply();
        tbl_no_idle_timeout_prop_0.apply();
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in EMPTY e, in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
