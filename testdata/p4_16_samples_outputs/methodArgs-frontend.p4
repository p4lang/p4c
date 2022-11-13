extern Random<T> {
    Random(T min);
    T read();
}

struct S {
    bit<32> f;
}

control c() {
    @name("c.r2") Random<bit<16>>(16w256) r2_0;
    apply {
        r2_0.read();
    }
}

control C();
package top(C _c);
top(c()) main;
