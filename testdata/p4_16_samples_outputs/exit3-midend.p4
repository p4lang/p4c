control ctrl(out bit<32> c) {
    bool hasExited;
    @name("ctrl.e") action e() {
        hasExited = true;
    }
    @name("ctrl.t") table t_0 {
        actions = {
            e();
        }
        default_action = e();
    }
    @hidden action exit3l24() {
        hasExited = false;
        c = 32w2;
    }
    @hidden action exit3l34() {
        c = 32w5;
    }
    @hidden table tbl_exit3l24 {
        actions = {
            exit3l24();
        }
        const default_action = exit3l24();
    }
    @hidden table tbl_exit3l34 {
        actions = {
            exit3l34();
        }
        const default_action = exit3l34();
    }
    apply {
        tbl_exit3l24.apply();
        t_0.apply();
        if (hasExited) {
            ;
        } else {
            tbl_exit3l34.apply();
        }
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;
