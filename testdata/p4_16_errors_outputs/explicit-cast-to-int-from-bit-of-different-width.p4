control p() {
    apply {
        bit<2> a;
        int<2> b;
        a = (bit<0>)b;
    }
}

