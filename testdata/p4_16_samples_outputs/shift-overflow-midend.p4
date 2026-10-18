header hdr_t {
    bit<8> v;
}

control c(inout hdr_t hdr) {
    @hidden action shiftoverflow7() {
        hdr.v = 8w0xf0;
    }
    @hidden table tbl_shiftoverflow7 {
        actions = {
            shiftoverflow7();
        }
        const default_action = shiftoverflow7();
    }
    apply {
        tbl_shiftoverflow7.apply();
    }
}

control C(inout hdr_t hdr);
package P(C c);
P(c()) main;
