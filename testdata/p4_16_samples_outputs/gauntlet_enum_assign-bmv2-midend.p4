#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct Headers {
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.do_thing") action do_thing() {
    }
    @hidden action gauntlet_enum_assignbmv2l15() {
        sm.egress_spec = 9w2;
    }
    @hidden table tbl_gauntlet_enum_assignbmv2l15 {
        actions = {
            gauntlet_enum_assignbmv2l15();
        }
        const default_action = gauntlet_enum_assignbmv2l15();
    }
    @hidden table tbl_do_thing {
        actions = {
            do_thing();
        }
        const default_action = do_thing();
    }
    apply {
        tbl_gauntlet_enum_assignbmv2l15.apply();
        tbl_do_thing.apply();
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
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
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
