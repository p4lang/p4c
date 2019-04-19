struct S {
    bit<32> f;
}

control caller() {
    S data_0;
    apply {
        data_0.f = 32w0;
        data_0.f = 32w0;
    }
}

control none();
package top(none n);
top(caller()) main;

