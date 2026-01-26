struct S {
    bit<1> a;
    bit<1> b;
}

control c() {
    apply {
        S s = { { ... }, ... };
    }
}

