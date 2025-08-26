typedef bit<1> t;
extern e {
    e();
}

parser MyParser() {
    e() ee = {
        t f(in .e a) {
            return a;
        }
    };
    state start {
    }
}

