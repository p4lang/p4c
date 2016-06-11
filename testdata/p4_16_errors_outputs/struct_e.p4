struct s {
}

control p() {
    apply {
        s v;
        bit<1> b;
        v.data = 1w0;
        b.data = 5;
    }
}

