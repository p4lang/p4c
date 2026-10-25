struct S {
    bit<32> a;
}

control c(inout S s) {
    @name("c.a") action a_1() {
        s.a = 32w1;
    }
    apply {
        a_1();
    }
}

control cc(inout S s);
package top(cc c);
top(c()) main;
