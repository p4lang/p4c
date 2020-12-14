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

extern void wg(out G g);

control c() {
    apply {
        U u;
        u.x = 2;
        bit<32> z = u.x; // not within a switch
        u.x.b = 0;       // not within a switch
        U u1 = U.x;
        switch (u) {
            U.G: { }          // no such field in union
            U.y: { u.x = 0; } // wrong fields accessed
            //U.z: { }          // no such field
            U1.x: { }         // incompatible label
            3: { }            // not a union field
            U.s: {
                switch (u1) {
                    U.x: { u.s.b = (bit<8>)u.y + (bit<8>)u1.x; }  // u.y is not guarded
                };
                u.s = { b = 2 };  // cannot be written while guarded
            }
        }
        switch (z) {
            U.y: { }          // wrong label type
        }

        G g;
        switch (g.u) {
            U.x: { wg(g); }   // g should not be mutable
        }
    }
}