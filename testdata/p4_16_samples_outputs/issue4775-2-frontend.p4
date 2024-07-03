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
    @name("c.retval") bit<8> retval;
    @name("c.retval_0") bit<8> retval_0;
    apply {
        if (hdr.isValid()) {
            retval = 8w1;
            hdr.value_6 = retval;
            retval_0 = 8w2;
            hdr.value_7 = retval_0;
        }
    }
}

control ctrl(inout hdr_t hdr);
package top(ctrl _c);
top(c()) main;
