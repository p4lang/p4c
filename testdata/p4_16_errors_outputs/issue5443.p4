#include <core.p4>

control MAPipe(inout bit<16> a);
package SimpleArch(MAPipe p);
control MyMAPipe(inout bit<16> a) {
    apply {
        if (~0x100) {
            a = 0;
        }
    }
}

SimpleArch(MyMAPipe()) main;
