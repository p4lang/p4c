header hdr_t {
    bit<8> v;
}

control c(inout hdr_t hdr) {
    apply {
        const int a = 5;
        const bit<4> b = 2;
        hdr.v = (bit<8>)(a >> b);
    }
}

control C(inout hdr_t hdr);
package P(C c);
P(c()) main;
