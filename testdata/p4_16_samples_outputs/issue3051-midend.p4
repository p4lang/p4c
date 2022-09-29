control C();
package P(C a);
control MyD() {
    @optional @name("MyD.b_0") bool b_0;
    @hidden action issue3051l6() {
        b_0 = !b_0;
    }
    @hidden table tbl_issue3051l6 {
        actions = {
            issue3051l6();
        }
        const default_action = issue3051l6();
    }
    apply {
        tbl_issue3051l6.apply();
    }
}

P(MyD()) main;

