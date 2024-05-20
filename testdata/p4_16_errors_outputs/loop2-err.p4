extern void fn(in bit<8> c);
control c() {
    apply {
        for (bit<8> a in 16w0 .. 16w15) {
            fn(a);
        }
    }
}

