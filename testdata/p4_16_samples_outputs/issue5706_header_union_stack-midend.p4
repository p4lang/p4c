#include <core.p4>


@command_line("--loopsUnroll") header h1 {
    bit<8> a;
}

header h2 {
    bit<16> b;
}

struct headers_t {
    h1 hus0_x;
    h2 hus0_y;
    h1 hus1_x;
    h2 hus1_y;
    h1 hus2_x;
    h2 hus2_y;
    h1 hus3_x;
    h2 hus3_y;
}

control C(inout headers_t hdrs);
package top(C c);
control c(inout headers_t hdrs) {
    @hidden action issue5706_header_union_stack24() {
        if (hdrs.hus2_x.isValid()) {
            hdrs.hus3_x.setValid();
            hdrs.hus3_x = hdrs.hus2_x;
            hdrs.hus3_y.setInvalid();
        } else {
            hdrs.hus3_x.setInvalid();
        }
        if (hdrs.hus2_y.isValid()) {
            hdrs.hus3_y.setValid();
            hdrs.hus3_y = hdrs.hus2_y;
            hdrs.hus3_x.setInvalid();
        } else {
            hdrs.hus3_y.setInvalid();
        }
        if (hdrs.hus1_x.isValid()) {
            hdrs.hus2_x.setValid();
            hdrs.hus2_x = hdrs.hus1_x;
            hdrs.hus2_y.setInvalid();
        } else {
            hdrs.hus2_x.setInvalid();
        }
        if (hdrs.hus1_y.isValid()) {
            hdrs.hus2_y.setValid();
            hdrs.hus2_y = hdrs.hus1_y;
            hdrs.hus2_x.setInvalid();
        } else {
            hdrs.hus2_y.setInvalid();
        }
        if (hdrs.hus0_x.isValid()) {
            hdrs.hus1_x.setValid();
            hdrs.hus1_x = hdrs.hus0_x;
            hdrs.hus1_y.setInvalid();
        } else {
            hdrs.hus1_x.setInvalid();
        }
        if (hdrs.hus0_y.isValid()) {
            hdrs.hus1_y.setValid();
            hdrs.hus1_y = hdrs.hus0_y;
            hdrs.hus1_x.setInvalid();
        } else {
            hdrs.hus1_y.setInvalid();
        }
        hdrs.hus0_x.setInvalid();
        hdrs.hus0_y.setInvalid();
        if (hdrs.hus2_x.isValid()) {
            hdrs.hus0_x.setValid();
            hdrs.hus0_x = hdrs.hus2_x;
            hdrs.hus0_y.setInvalid();
        } else {
            hdrs.hus0_x.setInvalid();
        }
        if (hdrs.hus2_y.isValid()) {
            hdrs.hus0_y.setValid();
            hdrs.hus0_y = hdrs.hus2_y;
            hdrs.hus0_x.setInvalid();
        } else {
            hdrs.hus0_y.setInvalid();
        }
        if (hdrs.hus3_x.isValid()) {
            hdrs.hus1_x.setValid();
            hdrs.hus1_x = hdrs.hus3_x;
            hdrs.hus1_y.setInvalid();
        } else {
            hdrs.hus1_x.setInvalid();
        }
        if (hdrs.hus3_y.isValid()) {
            hdrs.hus1_y.setValid();
            hdrs.hus1_y = hdrs.hus3_y;
            hdrs.hus1_x.setInvalid();
        } else {
            hdrs.hus1_y.setInvalid();
        }
        hdrs.hus2_x.setInvalid();
        hdrs.hus2_y.setInvalid();
        hdrs.hus3_x.setInvalid();
        hdrs.hus3_y.setInvalid();
    }
    @hidden table tbl_issue5706_header_union_stack24 {
        actions = {
            issue5706_header_union_stack24();
        }
        const default_action = issue5706_header_union_stack24();
    }
    apply {
        tbl_issue5706_header_union_stack24.apply();
    }
}

top(c()) main;
