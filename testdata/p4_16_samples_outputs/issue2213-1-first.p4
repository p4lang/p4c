struct mystruct1_t {
    bit<16> f1;
}

struct mystruct2_t {
    mystruct1_t s1;
}

struct metadata_t {
    mystruct1_t s1;
    mystruct2_t s2;
}

control ingressImpl(inout metadata_t meta) {
    mystruct2_t helper = mystruct2_t {s1 = mystruct1_t {f1 = 16w2}};
    apply {
        meta.s2 = helper;
    }
}

control c(inout metadata_t meta);
package top(c _c);
top(ingressImpl()) main;

