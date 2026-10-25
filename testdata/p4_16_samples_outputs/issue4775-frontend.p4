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
    @name("c.inlinedRetval") bit<8> inlinedRetval_1;
    @name("c.retval_0") bit<8> retval_0;
    @name("c.inlinedRetval_0") bit<8> inlinedRetval_2;
    apply {
        if (hdr.isValid()) {
            retval = 8w1;
            inlinedRetval_1 = retval;
            hdr.value_3 = inlinedRetval_1;
            retval_0 = 8w2;
            inlinedRetval_2 = retval_0;
            hdr.value_7 = inlinedRetval_2;
        }
    }
}

control ctrl(inout hdr_t hdr);
package top(ctrl _c);
top(c()) main;
