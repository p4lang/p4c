extern Random {
    Random(bit<16> min);
}

control c() {
    Random({#}) r;
    apply {
    }
}

