struct tuple_0 {
    bit<32> f0;
    bit<16> f1;
}

control c(out bit<16> r) {
    @hidden action tuple4l5() {
        r = 16w12;
    }
    @hidden table tbl_tuple4l5 {
        actions = {
            tuple4l5();
        }
        const default_action = tuple4l5();
    }
    apply {
        tbl_tuple4l5.apply();
    }
}

control _c(out bit<16> r);
package top(_c c);
top(c()) main;
