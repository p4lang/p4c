control SetAndFwd() {
    apply {
        bit<1> a;
        bit<1> b;
        a = (bit<1>)(bool)b;
    }
}

