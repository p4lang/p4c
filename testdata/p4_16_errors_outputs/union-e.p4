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

struct G {
    U u;
}

control c() {
    apply {
        U u;
        u.x = 2;
        bit<32> z = u.x;
        u.x.b = 0;
        U u1 = u.x;
        switch (u) {
            u.G: {
            }
            u.y as y: {
                u.x = 0;
            }
            u1.x: {
            }
            3: {
            }
            u.s: {
                switch (u1) {
                    u1.x as x: {
                        u.s.b = (bit<8>)u.y + (bit<8>)x;
                    }
                }
                ;
                u.s = {b = 2};
            }
        }
        switch (z) {
            u.y: {
            }
        }
        G g;
        switch (g.u) {
            g.u.x: 
            g.u.y: {
            }
        }
    }
}

