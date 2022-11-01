control C(out bit<16> result) {
    apply {
        typedef bit<32> T;
        bit<32> x = 32w5;
        {
            typedef bit<16> T;
            bit<16> y = 16w6;
            result = (bit<16>)x + y;
        }
    }
}

control CT(out bit<16> r);
package top(CT _c);
top(C()) main;

