#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<16> PortIdUint_t;
@p4runtime_translation("p4.org/psa/v1/PortId1_t" , bool) type PortIdUint_t PortId1_t;
@p4runtime_translation("p4.org/psa/v1/PortId2_t" , int < 32 >) type PortIdUint_t PortId2_t;
@p4runtime_translation("p4.org/psa/v1/PortId3_t" , bit < 5 + 3 >) type PortIdUint_t PortId3_t;
header ports_t {
    PortId1_t port1;
    PortId2_t port2;
    PortId3_t port3;
}

struct Headers {
    ports_t ports;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract(hdr.ports);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    table t {
        key = {
            h.ports.port1: exact;
            h.ports.port2: exact;
            h.ports.port3: exact;
        }
        actions = {
            NoAction;
        }
    }
    apply {
        t.apply();
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
