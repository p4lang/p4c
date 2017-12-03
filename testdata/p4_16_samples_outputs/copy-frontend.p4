struct S {
    bit<32> x;
}

control c(inout bit<32> b) {
    S s1_0;
    S s2_0;
    @name("a") action a_0() {
        s2_0 = { 32w0 };
        s1_0 = s2_0;
        s2_0 = s1_0;
        b = s2_0.x;
    }
    apply {
        a_0();
    }
}

control proto(inout bit<32> _b);
package top(proto _p);
top(c()) main;

