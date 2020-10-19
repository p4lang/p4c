control c(inout bit<16> y) {
    @name("c.x") bit<32> x_0;
    @name("c.a") action a() {
        y = (bit<16>)x_0;
    }
    @hidden action directaction18() {
        x_0 = 32w2;
    }
    @hidden table tbl_directaction18 {
        actions = {
            directaction18();
        }
        const default_action = directaction18();
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
    }
    apply {
        tbl_directaction18.apply();
        tbl_a.apply();
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;

