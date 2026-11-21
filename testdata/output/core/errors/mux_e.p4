control p() {
    apply {
        bit<1> a;
        bit<1> b;
        bit<1> c;
        bool d;
        a = (a ? b : c);
        d = (d ? a : d);
    }
}

