#include <core.p4>

bit<16> fun1(bit<8> args1) {
    return (bit<16>)args1;
}
bit<8> fun2(bit<4> args2) {
    fun1(8w10);
    return 8w0;
}

control ingress() {
    apply {
        fun2(4w3);
        fun2(4w3);
    }
}

control Ingress();
package top( Ingress ig);
top( ingress()) main;
