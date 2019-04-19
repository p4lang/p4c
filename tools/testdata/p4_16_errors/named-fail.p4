extern void f(in bit<16> x, in bool y);

control c() {
    apply {
        bit<16> xv = 0;
        bool    b  = true;

        f(y = b, xv);  // not all arguments named
        f(y = b, y = b); // same argument twice
    }
}
