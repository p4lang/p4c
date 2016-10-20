control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    bit<8> a_0;
    bit<16> b_0;
    bit<24> tmp;
    bit<32> tmp_0;
    bit<16> tmp_1;
    bit<32> tmp_2;
    bit<32> tmp_3;
    apply {
        a_0 = 8w0xf;
        b_0 = 16w0xf;
        tmp = a_0 ++ b_0;
        tmp_0 = tmp ++ a_0;
        tmp_1 = a_0 ++ a_0;
        tmp_2 = b_0 ++ tmp_1;
        tmp_3 = tmp_0 + tmp_2;
        x = tmp_3;
    }
}

top(c()) main;
