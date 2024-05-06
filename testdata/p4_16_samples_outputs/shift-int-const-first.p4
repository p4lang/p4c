header hdr_t {
    bit<8> v;
}

control c(inout hdr_t hdr) {
    apply {
        const int a = 5;
        const bit<4> b = 4w2;
        hdr.v = 8w1;
    }
}

control C(inout hdr_t hdr);
package P(C c);
P(c()) main;
