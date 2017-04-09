control c(out bit<32> y) {
    bit<32> x = 10;

    apply {
        y = (bit<32>)x;
    }
}