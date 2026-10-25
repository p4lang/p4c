control c(out bit<16> r) {
    apply {
        tuple<bit<32>, bit<16>> x = { 10, 12 };
        r = x[-2];
    }
}

