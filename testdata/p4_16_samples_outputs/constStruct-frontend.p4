struct S {
    bit<8> x;
}

control c(out bit<16> result) {
    apply {
        result = 16w0;
    }
}

control ctrl(out bit<16> result);
package top(ctrl _c);
top(c()) main;
