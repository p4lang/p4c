struct s {
    bit<1> f0;
    bit<1> f1;
}

extern bit<1> ext();
control c() {
    bit<2> tt;
    action a(in bit<1> v1, in bit<1> v2) {
        tt = v1 ++ v2;
    }
    action a1(in s v) {
        tt = v.f0 ++ v.f1;
    }
    table t {
        actions = {
            a1((s){f0 = ext(),f1 = ext()});
        }
        default_action = a1((s){f0 = ext(),f1 = ext()});
    }
    apply {
    }
}

