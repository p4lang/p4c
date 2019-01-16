header hdr {
    bit<1> g;
}

control A(inout hdr _hdr) {
    apply {
        _hdr.g = 1w1;
        _hdr.g = 1w1;
    }
}

control proto(inout hdr _hdr);
package top(proto p);
top(A()) main;

