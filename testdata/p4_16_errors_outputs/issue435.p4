struct mystruct1 {
    bit<4> a;
}

extern myExtern1 {
    myExtern1(bit<1> x);
    mystruct1(in bit<8> a, out bit<16> b);
}

control c() {
    myExtern1(1) m;
    apply {
    }
}

control ct();
package top(ct c);
top(c()) main;

