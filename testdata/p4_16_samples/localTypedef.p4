control C(out bit<16> result) {
    apply {
        typedef bit<32> T;
        T x = 5;
        {
            typedef bit<16> T;
            T y = 6;
            result = (bit<16>)x + y;
        }
    }
}

control CT(out bit<16> r);
package top(CT _c);

top(C()) main;
