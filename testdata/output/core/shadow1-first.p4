extern counter {
}

parser p() {
    bit<16> counter;
    state start {
        counter = 16w0;
        transition accept;
    }
}

