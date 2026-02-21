control proto<P>(inout P pkt);
package top<P>(proto<P> p);
header data_t {
    bit<8> e1;
    bit<8> e2;
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

struct headers {
    data_t data;
}

control c(inout headers hdr) {
    @hidden action enumCmp22() {
        hdr.data.a[0:0] = 1w1;
    }
    @hidden action enumCmp23() {
        hdr.data.a[1:1] = 1w1;
    }
    @hidden action enumCmp24() {
        hdr.data.a[2:2] = 1w1;
    }
    @hidden action enumCmp25() {
        hdr.data.a[3:3] = 1w1;
    }
    @hidden action enumCmp26() {
        hdr.data.a[4:4] = 1w1;
    }
    @hidden action enumCmp27() {
        hdr.data.a[5:5] = 1w1;
    }
    @hidden action enumCmp29() {
        hdr.data.b[1:1] = 1w1;
    }
    @hidden table tbl_enumCmp22 {
        actions = {
            enumCmp22();
        }
        const default_action = enumCmp22();
    }
    @hidden table tbl_enumCmp23 {
        actions = {
            enumCmp23();
        }
        const default_action = enumCmp23();
    }
    @hidden table tbl_enumCmp24 {
        actions = {
            enumCmp24();
        }
        const default_action = enumCmp24();
    }
    @hidden table tbl_enumCmp25 {
        actions = {
            enumCmp25();
        }
        const default_action = enumCmp25();
    }
    @hidden table tbl_enumCmp26 {
        actions = {
            enumCmp26();
        }
        const default_action = enumCmp26();
    }
    @hidden table tbl_enumCmp27 {
        actions = {
            enumCmp27();
        }
        const default_action = enumCmp27();
    }
    @hidden table tbl_enumCmp29 {
        actions = {
            enumCmp29();
        }
        const default_action = enumCmp29();
    }
    apply {
        if (hdr.data.e1 == hdr.data.e2) {
            tbl_enumCmp22.apply();
        }
        if (hdr.data.e1 == 8w1) {
            tbl_enumCmp23.apply();
        }
        if (hdr.data.e1 == 8w2) {
            tbl_enumCmp24.apply();
        }
        if (hdr.data.e1 == hdr.data.c) {
            tbl_enumCmp25.apply();
        }
        if (hdr.data.c == 8w2) {
            tbl_enumCmp26.apply();
        }
        if (hdr.data.c == 8w1) {
            tbl_enumCmp27.apply();
        }
        tbl_enumCmp29.apply();
    }
}

top<headers>(c()) main;
