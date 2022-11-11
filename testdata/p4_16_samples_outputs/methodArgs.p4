extern Random<T> {
    Random(T min);
}

struct S {
    bit<32> f;
}

control c() {
    Random<bit<16>>(1) r0;
    Random(16w256) r2;
    Random(true) r3;
    Random<bit<16>>(16w100) r1;
    Random<S>({f = 2}) r4;
    Random<S>({ 2 }) r5;
    apply {
    }
}

control C();
package top(C _c);
top(c()) main;
