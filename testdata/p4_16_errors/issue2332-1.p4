void f() {
    const int<8> x8 = 8s1;
    // RHS of shift cannot be signed.
    const int<8> shift8 = x8 << x8;
}