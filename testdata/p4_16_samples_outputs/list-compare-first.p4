typedef tuple<bit<32>, bit<32>> pair;
struct S {
    bit<32> l;
    bit<32> r;
}

const pair x = { 32w10, 32w20 };
const pair y = { 32w30, 32w40 };
const bool z = false;
const bool w = true;
const S s = { 32w10, 32w20 };
control c(out bool z);
package top(c _c);
control test(out bool zout) {
    apply {
        tuple<bit<32>, bit<32>> p = { 32w4, 32w5 };
        S q = { 32w2, 32w3 };
        zout = p == { 32w4, 32w5 };
        zout = zout && q == S {l = 32w2,r = 32w3};
    }
}

top(test()) main;

