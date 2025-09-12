extern void f<T, c>(in tuple<T> x);
control c() {
    apply {
        f<bit<32>>({ 32w2 });
    }
}

