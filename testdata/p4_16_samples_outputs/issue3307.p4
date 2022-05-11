typedef bit<1> t;
extern e {
    e();
    abstract t f(in t a);
}

parser MyParser(t tt) {
    e() ee = {
        t f(in t a) {
            return a;
        }
    };
    state start {
        transition accept;
    }
}

