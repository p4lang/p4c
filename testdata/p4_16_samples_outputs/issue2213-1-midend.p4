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
    @hidden action issue22131l18() {
        meta.s2.s1.f1 = 16w2;
    }
    @hidden table tbl_issue22131l18 {
        actions = {
            issue22131l18();
        }
        const default_action = issue22131l18();
    }
    apply {
        tbl_issue22131l18.apply();
    }
}

control c(inout metadata_t meta);
package top(c _c);
top(ingressImpl()) main;
