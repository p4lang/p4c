struct S {
    bit<8> b;
}

union U {
    bit<32> x;
    bit<16> y;
    S       s;
}

union U1 {
    bit<32> x;
}

control c() {
    apply {
        U u;
        u.x = 2;
        bit<32> z = u.x;
        u.x.b = 0;
        U u1 = U.x;
        switch (u) {
            U.y: {
                u.x = 0;
            }
            U1.x: {
            }
            3: {
            }
            U.s: {
                switch (u1) {
                    U.x: {
                        u.s.b = (bit<8>)u.y + (bit<8>)u1.x;
                    }
                }
                ;
                u.s = {b = 2};
            }
        }
        switch (z) {
            U.y: {
            }
        }
    }
}

