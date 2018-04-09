struct S {
    bit<32> f;
}

control caller() {
    S data;
    apply {
        data.f = 32w0;
        data.f = 32w0;
    }
}

control none();
package top(none n);
top(caller()) main;

