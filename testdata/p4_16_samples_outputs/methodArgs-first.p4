extern Random<T> {
    Random(T min);
}

struct S {
    bit<32> f;
}

control c() {
    Random<bit<16>>(16w1) r0;
    Random<bit<16>>(16w256) r2;
    Random<bool>(true) r3;
    Random<bit<16>>(16w100) r1;
    Random<S>((S){f = 32w2}) r4;
    Random<S>((S){f = 32w2}) r5;
    apply {
    }
}

control C();
package top(C _c);
top(c()) main;
