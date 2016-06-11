control p() {
    apply {
        bit<2> a;
        int<2> b;
        a = (bit<2>)b;
        b = (int<2>)a;
    }
}

