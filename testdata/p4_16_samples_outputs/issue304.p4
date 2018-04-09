control c(inout bit<32> y) {
    apply {
        y = y + 1;
    }
}

control t(inout bit<32> b) {
    c() c1;
    c() c2;
    apply {
        c1.apply(b);
        c2.apply(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

