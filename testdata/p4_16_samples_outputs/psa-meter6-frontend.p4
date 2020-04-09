#include <core.p4>
#include <psa.p4>

struct EMPTY {
}

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
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
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("MyIC.meter0") DirectMeter(PSA_MeterType_t.PACKETS) meter0_0;
    @name("MyIC.execute_meter") action execute_meter() {
        meter0_0.execute();
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            a.srcAddr: exact @name("a.srcAddr") ;
        }
        actions = {
            NoAction_0();
        }
        psa_direct_meter = meter0_0;
        default_action = NoAction_0();
    }
    @name("MyIC.tbl2") table tbl2_0 {
        key = {
            a.srcAddr: exact @name("a.srcAddr") ;
        }
        actions = {
            NoAction_3();
            execute_meter();
        }
        default_action = NoAction_3();
    }
    apply {
        tbl_0.apply();
        tbl2_0.apply();
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout ethernet_t d, in EMPTY e, in psa_ingress_output_metadata_t f) {
    apply {
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<ethernet_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;

EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;

PSA_Switch<ethernet_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

