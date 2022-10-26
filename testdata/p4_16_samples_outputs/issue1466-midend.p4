header hdr {
    bit<1> g;
}

control A(inout hdr _hdr) {
    @hidden action issue1466l7() {
        _hdr.g = 1w1;
    }
    @hidden table tbl_issue1466l7 {
        actions = {
            issue1466l7();
        }
        const default_action = issue1466l7();
    }
    apply {
        tbl_issue1466l7.apply();
    }
}

control proto(inout hdr _hdr);
package top(proto p);
top(A()) main;
