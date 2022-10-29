extern Generic<T> {
    Generic(T sz);
    R get<R>();
    R get1<R, S>(in S value, in R data);
}

extern void f<T>(in T arg);
control caller() {
    @name("caller.cinst.b") bit<5> cinst_b;
    @name("caller.cinst.x") Generic<bit<8>>(8w9) cinst_x;
    @hidden action generic1l28() {
        cinst_x.get<bit<32>>();
        cinst_b = cinst_x.get1<bit<5>, bit<10>>(10w0, 5w0);
        f<bit<5>>(cinst_b);
    }
    @hidden table tbl_generic1l28 {
        actions = {
            generic1l28();
        }
        const default_action = generic1l28();
    }
    apply {
        tbl_generic1l28.apply();
    }
}

control s();
package p(s parg);
p(caller()) main;
