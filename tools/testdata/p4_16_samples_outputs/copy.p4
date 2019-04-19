struct S {
    bit<32> x;
}

control c(inout bit<32> b) {
    action a() {
        S s1;
        S s2;
        s2 = { 0 };
        s1 = s2;
        s2 = s1;
        b = s2.x;
    }
    apply {
        a();
    }
}

control proto(inout bit<32> _b);
package top(proto _p);
top(c()) main;

