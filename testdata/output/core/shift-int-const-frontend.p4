header hdr_t {
    bit<8> v;
}

control c(inout hdr_t hdr) {
    apply {
        hdr.v = 8w1;
    }
}

control C(inout hdr_t hdr);
package P(C c);
P(c()) main;
