header H<T> {
    bit<32> b;
    T       t;
}

control c() {
    apply {
        match_kind s;
        s = (H<bit<32>>[3]){ { 0, 1 }, { 2, 3 }, (H<bit<32>>){#} };
    }
}

