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
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
<<<<<<< HEAD
    @name("ingress.key_0") bool key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
=======
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.key_0") bool key_0;
>>>>>>> One more test
    @name("ingress.sub_table") table sub_table_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name") ;
        }
        actions = {
<<<<<<< HEAD
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
=======
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
>>>>>>> One more test
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("dummy_name") ;
        }
        actions = {
<<<<<<< HEAD
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
=======
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
>>>>>>> One more test
    }
    apply {
        key_0 = sub_table_0.apply().hit;
        simple_table_0.apply();
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

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

