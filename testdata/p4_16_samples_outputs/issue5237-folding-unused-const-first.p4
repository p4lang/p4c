struct S {
    bit<32> a;
}

control c(inout S s) {
    const bit<32> one = 32w1;
    action a() {
        s.a = 32w1;
    }
    apply {
        a();
    }
}

control cc(inout S s);
package top(cc c);
top(c()) main;
