extern bit<32> f(in bit<32> x, out bit<16> y);
control c(out bit<32> r) {
    @name("c.arg") bit<16> arg;
    @name("c.tmp") bit<32> tmp;
    @hidden action issue3274l9() {
        tmp = f(x = 32w1, y = arg);
        r = tmp;
    }
    @hidden table tbl_issue3274l9 {
        actions = {
            issue3274l9();
        }
        const default_action = issue3274l9();
    }
    apply {
        tbl_issue3274l9.apply();
    }
}

control _c(out bit<32> r);
package top(_c _c);
top(c()) main;

