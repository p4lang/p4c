struct S {
    bit<32> l;
    bit<32> r;
}

control c(out bool z);
package top(c _c);
control test(out bool zout) {
    tuple<bit<32>, bit<32>> p_0;
    S q_0;
    apply {
        p_0 = { 32w4, 32w5 };
        q_0 = { 32w2, 32w3 };
        zout = p_0 == { 32w4, 32w5 };
        zout = zout && q_0 == {32w2,32w3};
    }
}

top(test()) main;

