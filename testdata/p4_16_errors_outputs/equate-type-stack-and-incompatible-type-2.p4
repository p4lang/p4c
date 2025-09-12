header H<T> {
    bit<32> b;
    T       t;
}

control c() {
    apply {
        tuple<bool> s;
        s = (H<bit<32>>[3]){ { 0, 1 }, { 2, 3 }, (H<bit<32>>){#} };
    }
}

