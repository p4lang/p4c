control c(inout bit<32> y) {
    apply {
    }
}

control t(inout bit<32> b) {
    c() c1;
    apply {
        c1.apply(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;
