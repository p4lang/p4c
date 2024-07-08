header hdr_t {
    bit<8> value_1;
    bit<8> value_2;
    bit<8> value_3;
    bit<8> value_4;
    bit<8> value_5;
    bit<8> value_6;
    bit<8> value_7;
}

control c(inout hdr_t hdr) {
    @hidden action issue4775l17() {
        hdr.value_3 = 8w1;
        hdr.value_7 = 8w2;
    }
    @hidden table tbl_issue4775l17 {
        actions = {
            issue4775l17();
        }
        const default_action = issue4775l17();
    }
    apply {
        if (hdr.isValid()) {
            tbl_issue4775l17.apply();
        }
    }
}

control ctrl(inout hdr_t hdr);
package top(ctrl _c);
top(c()) main;
