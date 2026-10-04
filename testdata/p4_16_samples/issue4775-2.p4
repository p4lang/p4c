bit<8> add_1(in bit<8> a, in bit<8> b) {
    return 1;
}
bit<8> add_1(in bit<8> c, in bit<8> d) {
    return 2;
}
header hdr_t {
    bit<8> value_1;
    bit<8> value_2;
    bit<8> value_3;
    bit<8> value_4;
    bit<8> value_5;
    bit<8> value_6;
    bit<8> value_7;
}

control c(inout hdr_t hdr) {
    apply {
        if (hdr.isValid()) {
            hdr.value_6 = add_1(a = hdr.value_1, b = hdr.value_2);
            hdr.value_7 = add_1(d = hdr.value_3, c = hdr.value_4);
        }
    }
}

control ctrl(inout hdr_t hdr);
package top(ctrl _c);
top(c()) main;
