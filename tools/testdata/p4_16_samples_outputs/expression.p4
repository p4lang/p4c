control p() {
    apply {
        int<32> a = 32s1;
        int<32> b = 32s1;
        int<32> c;
        bit<32> f;
        bit<16> e;
        bool d;
        c = b;
        c = -b;
        f = ~(bit<32>)b;
        f = (bit<32>)a & (bit<32>)b;
        f = (bit<32>)a | (bit<32>)b;
        f = (bit<32>)a ^ (bit<32>)b;
        f = (bit<32>)a << (bit<32>)b;
        f = (bit<32>)a >> (bit<32>)b;
        f = (bit<32>)a >> 4;
        f = (bit<32>)a << 6;
        c = a * b;
        e = ((bit<32>)a)[15:0];
        f = e ++ e;
        c = (d ? a : b);
        d = a == b;
        d = a != b;
        d = a < b;
        d = a > b;
        d = a <= b;
        d = a >= b;
        d = !d;
        d = d && d;
        d = d || d;
        d = d == d;
        d = d != d;
    }
}

