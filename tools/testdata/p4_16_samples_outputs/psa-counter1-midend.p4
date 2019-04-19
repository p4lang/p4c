#include <core.p4>
#include <psa.p4>

struct EMPTY {
}

header EMPTY_H {
}

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

parser MyIP(packet_in buffer, out ethernet_t eth, inout EMPTY b, in psa_ingress_parser_input_metadata_t c, in EMPTY_H d, in EMPTY_H e) {
    state start {
        buffer.extract<ethernet_t>(eth);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY_H e, in EMPTY_H f) {
    state start {
        transition accept;
    }
}

control MyIC(inout ethernet_t a, inout EMPTY b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("MyIC.counter") Counter<bit<10>, bit<12>>(32w1024, PSA_CounterType_t.PACKETS) counter_0;
    @name("MyIC.execute") action execute_1() {
        counter_0.count(12w1024);
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            a.srcAddr: exact @name("a.srcAddr") ;
        }
        actions = {
            NoAction_0();
            execute_1();
        }
        default_action = NoAction_0();
    }
    apply {
        tbl_0.apply();
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY_H a, out EMPTY_H b, out EMPTY c, inout ethernet_t d, in EMPTY e, in psa_ingress_output_metadata_t f) {
    apply {
    }
}

control MyED(packet_out buffer, out EMPTY_H a, out EMPTY_H b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<ethernet_t, EMPTY, EMPTY, EMPTY_H, EMPTY_H, EMPTY_H>(MyIP(), MyIC(), MyID()) ip;

EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY_H, EMPTY_H, EMPTY_H>(MyEP(), MyEC(), MyED()) ep;

PSA_Switch<ethernet_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY_H, EMPTY_H, EMPTY_H, EMPTY_H>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

