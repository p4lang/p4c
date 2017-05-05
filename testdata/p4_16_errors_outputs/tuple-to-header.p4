header H {
    bit<32> x;
}

control c() {
    tuple<bit<32>> t = { 0 };
    H h;
    apply {
        h = t;
    }
}

