struct S {
    bit<32> f;
}

control c(inout S data) {
    apply {
        data.f = 32w0;
    }
}

control caller() {
    S data_0;
    @name("cinst") c() cinst_0;
    apply {
        data_0.f = 32w0;
        cinst_0.apply(data_0);
    }
}

control none();
package top(none n);
top(caller()) main;

