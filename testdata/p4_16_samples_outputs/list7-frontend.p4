extern E<V> {
    E(V param);
    void run();
}

struct S {
    bit<32> f;
}

control c() {
    @name("c.e") E<S>((S){f = 32w5}) e_0;
    apply {
        e_0.run();
    }
}

control C();
package top(C _c);
top(c()) main;
