bit<8> add(in bit<8> a, in bit<8> b) {
    return 1;
}
bit<8> add(in bit<8> a, in bit<8> b, in bit<8> c) {
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
            hdr.value_3 = add(hdr.value_1, hdr.value_2);
            hdr.value_7 = add(hdr.value_4, hdr.value_5, hdr.value_6);
        }
    }
}

control ctrl(inout hdr_t hdr);
package top(ctrl _c);
top(c()) main;
