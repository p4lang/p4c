struct s {
    bit<1> z;
    bit<1> w;
}

struct t {
    s s1;
    s s2;
}

extern bit<1> fct(in bit<1> a, in bit<1> b);
