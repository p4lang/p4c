header hdr {
    bit<32> a;
}

control ct(out bit<32> x, out bit<32> y);
package top(ct _ct);
control c(out bit<32> x, out bit<32> y) {
    @name("c.h") hdr h_0;
    @name("c.b") bool b_0;
    @name("c.simple_action") action simple_action() {
        y = x;
        x = 32w0;
    }
    @hidden action simplify_method_calls42() {
        x = h_0.a;
    }
    @hidden action simplify_method_calls52() {
        h_0.a = 32w0;
        x = 32w0;
    }
    @hidden action simplify_method_calls28() {
        h_0.setValid();
        h_0.a = 32w0;
        b_0 = h_0.isValid();
        h_0.setValid();
    }
    @hidden table tbl_simplify_method_calls28 {
        actions = {
            simplify_method_calls28();
        }
        const default_action = simplify_method_calls28();
    }
    @hidden table tbl_simplify_method_calls42 {
        actions = {
            simplify_method_calls42();
        }
        const default_action = simplify_method_calls42();
    }
    @hidden table tbl_simplify_method_calls52 {
        actions = {
            simplify_method_calls52();
        }
        const default_action = simplify_method_calls52();
    }
    @hidden table tbl_simple_action {
        actions = {
            simple_action();
        }
        const default_action = simple_action();
    }
    apply {
        tbl_simplify_method_calls28.apply();
        if (b_0) {
            tbl_simplify_method_calls42.apply();
        } else {
            tbl_simplify_method_calls52.apply();
        }
        tbl_simple_action.apply();
    }
}

top(c()) main;
