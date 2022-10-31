typedef bit<32> T;
header H {
}

control c(out bit<32> v) {
    apply {
        bit<32> b;
        H[0] h;
        v = b.minSizeInBits() + T.minSizeInBits() + b.maxSizeInBits() + T.maxSizeInBits() + h[0].minSizeInBits();
    }
}

const int w = T.maxSizeInBits();
control cproto(out bit<32> v);
package top(cproto _c);
top(c()) main;
