extern void f<T>(in tuple<T> x);
control c() {
    apply {
        f<match_kind>({ 32w2 });
    }
}

