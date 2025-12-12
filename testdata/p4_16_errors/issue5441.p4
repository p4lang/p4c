#include <core.p4>

control MAPipe(out bit<16> a, in bit<32> b);
package SimpleArch(MAPipe p);

control MyMAPipe(out bit<16> a, in bit<32> b) {
    apply {
        a = b[-1+:16];
        a = b[4294967293+:16];
    }
}

SimpleArch(MyMAPipe()) main;
