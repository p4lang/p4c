#include <core.p4>
#include <bmv2/psa.p4>

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

struct headers_t {
    ethernet_t ethernet;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout EMPTY b, in psa_ingress_parser_input_metadata_t c, in EMPTY_H d, in EMPTY_H e) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY_H e, in EMPTY_H f) {
    state start {
        transition accept;
    }
}

type bit<12> CounterIndex_t;
control MyIC(inout headers_t hdr, inout EMPTY b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIC.counter") Counter<bit<10>, CounterIndex_t>(32w1024, PSA_CounterType_t.PACKETS) counter_0;
    @name("MyIC.execute") action execute_1() {
        counter_0.count((CounterIndex_t)12w1024);
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            execute_1();
        }
        default_action = NoAction_1();
    }
    apply {
        tbl_0.apply();
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY_H a, out EMPTY_H b, out EMPTY c, inout headers_t hdr, in EMPTY e, in psa_ingress_output_metadata_t f) {
    apply {
    }
}

control MyED(packet_out buffer, out EMPTY_H a, out EMPTY_H b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, EMPTY, EMPTY, EMPTY_H, EMPTY_H, EMPTY_H>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY_H, EMPTY_H, EMPTY_H>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY_H, EMPTY_H, EMPTY_H, EMPTY_H>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
