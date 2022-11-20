extern E<V> {
    E(V param);
    void run();
}

struct S {
    bit<32> f;
}

control c() {
    E<S>({ f = 5 }) e;

    apply {
        e.run();
    }
}

control C();
package top(C _c);

top(c()) main;
