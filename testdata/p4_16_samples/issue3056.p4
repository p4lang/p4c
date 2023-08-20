enum bit<8> E {
  e1 = 1,
  e2 = 2
}

control compute() {
    apply {
        E e = E.e1;
        bit<8> x1 = 8w0 << e;
        bit<8> x2 = -e;
        bit<4> x3 = e[3:0];
        bool x4 = 8w0 == e;
        bit<8> x5 = 8w0 | e;
        bit<8> x6 = e << 3;     // p4c error: "<< left operand of shift must be a numeric type, not E"
        bit<16> x7 = e ++ 8w0;  // p4c error: "Concatenation not defined on E and bit<8>"
        bit<4> x8 = 8w0[4:E.e1];   // p4c error: "bit indexes must be compile-time constants"
    }
}