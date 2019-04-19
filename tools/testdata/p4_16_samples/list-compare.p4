typedef tuple<bit<32>, bit<32>> pair;

struct S {
    bit<32> l;
    bit<32> r;
}

const pair x = { 10, 20 };
const pair y = { 30, 40 };
const bool z = x == y;
const bool w = x == x;
const S s = { 10, 20 };

control c(out bool z);
package top(c _c);

control test(out bool zout) {
    apply {
        pair p = { 4, 5 };
        S q = { 2, 3 };

        zout = p == { 4, 5 };
        zout = zout && (q == { 2, 3 });
    }
}

top(test()) main;
