struct S {
    bit<32> f;
}

control caller() {
    apply {
    }
}

control none();
package top(none n);
top(caller()) main;

