header h {
    bit<32> field;
}

control c() {
    h hdr;
    apply {
        hdr = (h){field = 32w10};
    }
}

