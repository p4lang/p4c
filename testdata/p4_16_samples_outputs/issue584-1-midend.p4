#include <core.p4>
#include <v1model.p4>

control p();
package top(p _p);
control c() {
    bit<16> var_0;
    @hidden action act() {
        hash<bit<16>, bit<16>, bit<32>, bit<16>>(var_0, HashAlgorithm.crc16, 16w0, 32w0, 16w0xffff);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

top(c()) main;

