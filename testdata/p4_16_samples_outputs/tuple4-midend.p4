struct tuple_0 {
    bit<32> f0;
    bit<16> f1;
}

control c(out bit<16> r) {
    @hidden action tuple4l13() {
        r = 16w12;
    }
    @hidden table tbl_tuple4l13 {
        actions = {
            tuple4l13();
        }
        const default_action = tuple4l13();
    }
    apply {
        tbl_tuple4l13.apply();
    }
}

control _c(out bit<16> r);
package top(_c c);
top(c()) main;
