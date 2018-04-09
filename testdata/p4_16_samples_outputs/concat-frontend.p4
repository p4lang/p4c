control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    bit<8> a;
    bit<16> b;
    apply {
        a = 8w0xf;
        b = 16w0xf;
        x = a ++ b ++ a + (b ++ (a ++ a));
    }
}

top(c()) main;

