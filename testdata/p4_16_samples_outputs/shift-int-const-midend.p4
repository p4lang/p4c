header hdr_t {
    bit<8> v;
}

control c(inout hdr_t hdr) {
    @hidden action shiftintconst9() {
        hdr.v = 8w1;
    }
    @hidden table tbl_shiftintconst9 {
        actions = {
            shiftintconst9();
        }
        const default_action = shiftintconst9();
    }
    apply {
        tbl_shiftintconst9.apply();
    }
}

control C(inout hdr_t hdr);
package P(C c);
P(c()) main;
