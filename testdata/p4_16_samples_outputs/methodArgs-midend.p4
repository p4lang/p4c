extern Random<T> {
    Random(T min);
    T read();
}

struct S {
    bit<32> f;
}

control c() {
    @name("c.r2") Random<bit<16>>(16w256) r2_0;
    @hidden action methodArgs13() {
        r2_0.read();
    }
    @hidden table tbl_methodArgs13 {
        actions = {
            methodArgs13();
        }
        const default_action = methodArgs13();
    }
    apply {
        tbl_methodArgs13.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
