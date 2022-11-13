extern Random<T> {
    Random(T min);
    T read();
}

struct S {
    bit<32> f;
}

control c() {
    Random(16w256) r2;
    apply {
        bit<16> v = r2.read();
    }
}

control C();
package top(C _c);

top(c()) main;
