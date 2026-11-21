struct s {
    bit<1> f0;
    bit<1> f1;
}

extern bit<1> ext1();
extern bit<1> ext2();
control c() {
    bit<2> tt;
    action a1(in s v) {
        tt = v.f0 ++ v.f1;
    }
    table t {
        actions = {
            a1({f0 = ext1(),f1 = ext2()});
        }
        default_action = a1({f1 = ext1(),f0 = ext2()});
    }
    apply {
    }
}

