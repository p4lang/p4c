#include <core.p4>

struct standard_metadata_t {
}

header data_h {
    bit<32> da;
    bit<32> db;
}

struct my_packet {
    data_h data;
}

control c(in my_packet hdr) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("c.nop") action nop() {
    }
    @name("c.t") table t_0 {
        actions = {
            nop();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.db: exact @name("hdr.data.db") ;
        }
        default_action = NoAction_0();
    }
    apply {
        if (hdr.data.da == 32w1) 
            t_0.apply();
    }
}

control C(in my_packet hdr);
package V1Switch(C vr);
V1Switch(c()) main;

