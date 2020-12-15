#include <core.p4>
#include <psa.p4>

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

parser MyIP(packet_in buffer, out ethernet_t eth, inout EMPTY b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        buffer.extract<ethernet_t>(eth);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout ethernet_t a, inout EMPTY b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @name("MyIC.a1") action a1(bit<48> param) {
        a.dstAddr = param;
    }
    @name("MyIC.a1") action a1_3(bit<48> param) {
        a.dstAddr = param;
    }
    @name("MyIC.a1") action a1_4(bit<48> param) {
        a.dstAddr = param;
    }
    @name("MyIC.a2") action a2(bit<16> param) {
        a.etherType = param;
    }
    @name("MyIC.a2") action a2_3(bit<16> param) {
        a.etherType = param;
    }
    @name("MyIC.a2") action a2_4(bit<16> param) {
        a.etherType = param;
    }
    @name("MyIC.tbl_idle_timeout") table tbl_idle_timeout_0 {
        key = {
            a.srcAddr: exact @name("a.srcAddr") ;
        }
        actions = {
            NoAction_0();
            a1();
            a2();
        }
        psa_idle_timeout = PSA_IdleTimeout_t.NOTIFY_CONTROL;
        default_action = NoAction_0();
    }
    @name("MyIC.tbl_no_idle_timeout") table tbl_no_idle_timeout_0 {
        key = {
            a.srcAddr2: exact @name("a.srcAddr2") ;
        }
        actions = {
            NoAction_4();
            a1_3();
            a2_3();
        }
        psa_idle_timeout = PSA_IdleTimeout_t.NO_TIMEOUT;
        default_action = NoAction_4();
    }
    @name("MyIC.tbl_no_idle_timeout_prop") table tbl_no_idle_timeout_prop_0 {
        key = {
            a.srcAddr2: exact @name("a.srcAddr2") ;
        }
        actions = {
            NoAction_5();
            a1_4();
            a2_4();
        }
        default_action = NoAction_5();
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

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout ethernet_t d, in EMPTY e, in psa_ingress_output_metadata_t f) {
    @hidden action psaidletimeout98() {
        buffer.emit<ethernet_t>(d);
    }
    @hidden table tbl_psaidletimeout98 {
        actions = {
            psaidletimeout98();
        }
        const default_action = psaidletimeout98();
    }
    apply {
        tbl_psaidletimeout98.apply();
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<ethernet_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;

EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;

PSA_Switch<ethernet_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

