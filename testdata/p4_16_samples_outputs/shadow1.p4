extern counter {
}

parser p() {
    bit<16> counter;
    state start {
        counter = 0;
        transition accept;
    }
}

