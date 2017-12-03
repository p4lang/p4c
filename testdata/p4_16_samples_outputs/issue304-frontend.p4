control c(inout bit<32> y) {
    apply {
        y = y + 32w1;
    }
}

control t(inout bit<32> b) {
    @name("c1") c() c1_0;
    @name("c2") c() c2_0;
    apply {
        c1_0.apply(b);
        c2_0.apply(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

