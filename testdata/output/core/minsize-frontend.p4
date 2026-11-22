header H {
}

control c(out bit<32> v) {
    apply {
        v = 32w128;
    }
}

control cproto(out bit<32> v);
package top(cproto _c);
top(c()) main;
