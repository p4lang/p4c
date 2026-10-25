typedef bit<1> t;
extern e {
    e();
    abstract t f(in match_kind a);
}

parser MyParser0() {
    e() ee = {
        t f(in t a) {
            return a;
        }
    };
    state start {
    }
}

