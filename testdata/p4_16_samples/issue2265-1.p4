extern void f<T>(in tuple<T> x);

control c() {
    apply {
        f<bit<32>>({ 32w2 });
    }
}