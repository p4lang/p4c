extern ext<H, V> {
    ext(H v);
    V method1<T>(in H h, in T t);
    H method2<T>(in T t);
}

control c() {
    ext<bit<16>, void>(16w0) x;
    apply {
        x.method1<bit<8>>(x.method2<bit<12>>(12w1), 8w0);
    }
}

