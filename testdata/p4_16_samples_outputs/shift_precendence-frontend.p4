control i(out bit<4> a, out bit<16> x) {
    @name("i.tmp_0") bit<4> tmp;
    apply {
        tmp = 4w0;
        a = tmp + 4w2 >> 4w2 & 4w1;
        x = 16w0xfff;
    }
}

control c(out bit<4> a, out bit<16> x);
package top(c _c);
top(i()) main;
