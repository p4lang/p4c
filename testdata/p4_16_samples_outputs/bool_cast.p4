control SetAndFwd() {
    apply {
        bit<1> a;
        bool d;
        a = (bit<1>)d;
        d = (bool)a;
    }
}

