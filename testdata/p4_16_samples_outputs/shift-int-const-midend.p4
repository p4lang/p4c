header hdr_t {
    bit<8> v;
}

control c(inout hdr_t hdr) {
    @hidden action shiftintconst15() {
        hdr.v = 8w1;
    }
    @hidden table tbl_shiftintconst15 {
        actions = {
            shiftintconst15();
        }
        const default_action = shiftintconst15();
    }
    apply {
        tbl_shiftintconst15.apply();
    }
}

control C(inout hdr_t hdr);
package P(C c);
P(c()) main;
