struct S {}
header hdr {}
    
control p()
{
    apply {
        int<2> a;
        int<4> c;
        bool d;
        bit<2> e;
        bit<4> f;
        hdr[5] stack;
    
        S g;
        S h;

        c = a[2];   // not an array    
        c = stack[d];   // indexing with bool
    
        f = e & f;  // different width
        d = g == h; // not defined on structs
    
        d = d < d;  // not defined on bool
        d = a > c;  // different width
    }
}
