extern void f<T>(in list<bool> x);
control c() {
    apply {
        f<bit<32>>({ 32w2 });
    }
}

