extern X {
    X();
    abstract void a(inout bit<32> arg);
}

control c(inout bit<32> y) {
    @name("c.x") X() x_0 = {
        void a(inout bit<32> arg) {
            arg = arg + 32w1;
        }
    };
    @hidden action issue21755l29() {
        x_0.a(y);
    }
    @hidden table tbl_issue21755l29 {
        actions = {
            issue21755l29();
        }
        const default_action = issue21755l29();
    }
    apply {
        tbl_issue21755l29.apply();
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(c()) main;
