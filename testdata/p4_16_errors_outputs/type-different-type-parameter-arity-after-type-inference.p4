extern void f<T>(out tuple<> lazy);
control c() {
    tuple<bit<32>, bool> x = { 10, false };
    apply {
        f(x);
    }
}

