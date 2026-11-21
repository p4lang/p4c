header hdr_t {
    bit<8> v;
}

control c(inout hdr_t hdr) {
    apply {
        hdr.v = 8w0xf0;
    }
}

control C(inout hdr_t hdr);
package P(C c);
P(c()) main;
