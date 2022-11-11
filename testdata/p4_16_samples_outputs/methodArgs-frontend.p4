extern Random<T> {
    Random(T min);
}

struct S {
    bit<32> f;
}

control c() {
    @name("c.r0") Random<bit<16>>(16w1) r0_0;
    @name("c.r2") Random<bit<16>>(16w256) r2_0;
    @name("c.r3") Random<bool>(true) r3_0;
    @name("c.r1") Random<bit<16>>(16w100) r1_0;
    @name("c.r4") Random<S>((S){f = 32w2}) r4_0;
    @name("c.r5") Random<S>((S){f = 32w2}) r5_0;
    apply {
    }
}

control C();
package top(C _c);
top(c()) main;
