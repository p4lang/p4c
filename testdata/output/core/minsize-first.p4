typedef bit<32> T;
header H {
}

control c(out bit<32> v) {
    apply {
        bit<32> b;
        H[0] h;
        v = 32w128;
    }
}

const int w = 32;
control cproto(out bit<32> v);
package top(cproto _c);
top(c()) main;
