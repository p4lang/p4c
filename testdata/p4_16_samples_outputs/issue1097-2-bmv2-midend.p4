#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header myhdr_t {
    bit<8> reg_idx_to_update;
    bit<8> value_to_add;
    bit<8> debug_last_reg_value_written;
}

struct Headers {
    myhdr_t myhdr;
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract<myhdr_t>(h.myhdr);
        transition accept;
    }
}

register<bit<8>>(32w256) r;
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action issue10972bmv2l52() {
        r.write((bit<32>)h.myhdr.reg_idx_to_update, 8w0x2a);
    }
    @hidden table tbl_issue10972bmv2l52 {
        actions = {
            issue10972bmv2l52();
        }
        const default_action = issue10972bmv2l52();
    }
    apply {
        tbl_issue10972bmv2l52.apply();
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("egress.tmp") bit<8> tmp_0;
    @hidden action issue10972bmv2l62() {
        r.read(tmp_0, (bit<32>)h.myhdr.reg_idx_to_update);
        tmp_0 = tmp_0 + h.myhdr.value_to_add;
        r.write((bit<32>)h.myhdr.reg_idx_to_update, tmp_0);
        h.myhdr.debug_last_reg_value_written = tmp_0;
    }
    @hidden table tbl_issue10972bmv2l62 {
        actions = {
            issue10972bmv2l62();
        }
        const default_action = issue10972bmv2l62();
    }
    apply {
        tbl_issue10972bmv2l62.apply();
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<myhdr_t>(h.myhdr);
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

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
