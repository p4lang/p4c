control qp() {
    action drop() {
    }
    table m {
        actions = {
            drop;
        }
        default_action = drop;
    }
    apply {
        m.apply();
    }
}

extern Ix {
    Ix();
    void f();
    void f1(in int<32> x);
    void g();
    int<32> h();
}

control p() {
    Ix() x;
    Ix() y;
    action b(in bit<32> arg2) {
    }
    apply {
        x.f();
        x.f1(32s1);
        b(32w0);
    }
}

