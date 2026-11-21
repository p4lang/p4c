#include <core.p4>

control MAPipe<M>(inout M meta);
package SimpleArch<H>(MAPipe<H> mapipe);
struct metadata {
    bit<8> f0;
}

control MyIngress(inout metadata meta) {
    apply {
        meta.f0 = 7[1:-1];
    }
}

SimpleArch(MyIngress()) main;
