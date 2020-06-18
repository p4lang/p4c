control ctrl(out bit<32> c) {
    @name("ctrl.e") action e() {
    }
    @name("ctrl.t") table t_0 {
        actions = {
            e();
        }
        default_action = e();
    }
    @hidden action exit3l43() {
        c = 32w5;
    }
    @hidden table tbl_exit3l43 {
        actions = {
            exit3l43();
        }
        const default_action = exit3l43();
    }
    apply {
        t_0.apply();
        tbl_exit3l43.apply();
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;

