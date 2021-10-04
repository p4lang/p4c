typedef bit<32> T;
control c(out bit<32> v) {
    apply {
        bit<32> b;
        v = b.minSizeInBits() + T.minSizeInBits();
    }
}

control cproto(out bit<32> v);
package top(cproto _c);
top(c()) main;

