struct S {
    bit<8>  a;
    bit<16> b;
}

struct N {
    S    s;
    bool f;
}

const S s1 = (S){a = 8w1,b = 16w2};
const S s2 = (S){a = 8w1,b = 16w3};
const S s1r = (S){b = 16w2,a = 8w1};
const N n1 = (N){s = (S){a = 8w1,b = 16w2},f = true};
const N n2 = (N){s = (S){b = 16w2,a = 8w1},f = true};
const bool t1 = false;
const bool t2 = true;
const bool t3 = true;
const bool t4 = false;
const bool t5 = true;
const bool t6 = true;
const bool t7 = true;
const bool t8 = true;
control c(out bool z);
package top(c _c);
control test(out bool zout) {
    apply {
        zout = true;
    }
}

top(test()) main;
