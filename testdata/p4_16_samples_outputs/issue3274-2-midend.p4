extern bit<32> f(in bit<32> x, out bit<16> y);
control c() {
    @name("c.arg") bit<16> arg;
    @hidden action issue32742l10() {
        f(x = 32w1, y = arg);
    }
    @hidden table tbl_issue32742l10 {
        actions = {
            issue32742l10();
        }
        const default_action = issue32742l10();
    }
    apply {
        tbl_issue32742l10.apply();
    }
}

control _c();
package top(_c _c);
top(c()) main;
