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
        bit<32> z = u.x; // not within a switch
        u.x.b = 0;       // not within a switch
        U u1 = u.x;
        switch (u) {
            u.G: { }          // no such field in union
            u.y as y: { u.x = 0; } // wrong fields accessed
            //U.z: { }          // no such field
            u1.x: { }         // incompatible label
            3: { }            // not a union field
            u.s: {
                switch (u1) {
                    u1.x as x: { u.s.b = (bit<8>)u.y + (bit<8>)x; }  // u.y is not guarded
                };
                u.s = { b = 2 };  // cannot be written while guarded
            }
        }
        switch (z) {
            u.y: { }          // wrong label type
        }

        G g;
        switch (g.u) {
            g.u.x:
            g.u.y: { } // no fallthroguh allowed for unions
        }
    }
}