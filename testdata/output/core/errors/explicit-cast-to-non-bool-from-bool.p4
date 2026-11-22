control SetAndFwd() {
    apply {
        bit<1> a;
        bit<1> b;
        a = (int<1>)b;
    }
}

