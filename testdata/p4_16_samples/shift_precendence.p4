control i(out bit<4> a, out bit<16> x) {
    bit<4> tmp_0;
    apply {
        tmp_0 = 4w0;
        a = 4w1 & (4w2 + tmp_0) >> 4w2;
        x = 0xffff >> 3 >> 1;
    }
}

control c(out bit<4> a, out bit<16> x);
package top(c _c);
top(i()) main;

