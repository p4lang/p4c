#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    @name("p.e") ethernet_t e_0;
    state start {
        e_0.setInvalid();
        pkt.extract<ethernet_t>(e_0);
        transition select(e_0.eth_type) {
            16w0x800: accept;
            16w0x806: accept;
            default: reject;
        }
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_act") action do_act(@name("type") bit<32> type_1) {
        sm.instance_type = type_1;
    }
    @name("ingress.tns") table tns_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("h.eth_hdr.eth_type");
        }
        actions = {
            do_act();
            @defaultonly NoAction_1();
        }
        const entries = {
                        16w0x800 : do_act(32w0x800);
                        16w0x8100 : do_act(32w0x8100);
        }
        default_action = NoAction_1();
    }
    apply {
        tns_0.apply();
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
