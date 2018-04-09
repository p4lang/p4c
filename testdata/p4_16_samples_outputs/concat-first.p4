control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    apply {
        bit<8> a = 8w0xf;
        bit<16> b = 16w0xf;
        x = a ++ b ++ a + (b ++ (a ++ a));
    }
}

top(c()) main;

