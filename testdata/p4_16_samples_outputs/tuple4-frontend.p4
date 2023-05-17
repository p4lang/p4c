control c(out bit<16> r) {
    @name("c.x") tuple<bit<32>, bit<16>> x_0;
    apply {
        x_0 = { 32w10, 16w12 };
        if (x_0 == { 32w10, 16w12 }) {
            r = x_0[1];
        } else {
            r = (bit<16>)x_0[0];
        }
    }
}

control _c(out bit<16> r);
package top(_c c);
top(c()) main;
