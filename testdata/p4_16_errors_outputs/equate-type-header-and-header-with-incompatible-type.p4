header H<T> {
    bit<32> b;
    T       t;
}

control c() {
    apply {
        H<bit<32>>[3] s;
        s = (H<bit<32>>[3]){ { 0, 1 }, { 2, 3 }, (H<bit<2>>){#} };
    }
}

