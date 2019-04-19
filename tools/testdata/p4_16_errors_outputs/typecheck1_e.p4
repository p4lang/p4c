control p() {
    apply {
        bit<32> a;
        int<32> b;
        bit<32> c;
        bool d;
        b = ~b;
        c = ~c;
        d = ~d;
        a = a + a;
        a = a - a;
        d = d + d;
        d = d - d;
        a = !a;
        b = !b;
    }
}

