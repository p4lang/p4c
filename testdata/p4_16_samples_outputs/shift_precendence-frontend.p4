control i(out bit<4> a) {
    bit<4> tmp;
    apply {
        tmp = 4w0;
        a = 4w1 & 4w2 + tmp >> 4w2;
    }
}

control c(out bit<4> a);
package top(c _c);
top(i()) main;

