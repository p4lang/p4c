control ctrl(out bit<32> c) {
    bool hasExited;
    @name("ctrl.e") action e() {
        hasExited = true;
    }
    @hidden action exit2l19() {
        hasExited = false;
        c = 32w2;
    }
    @hidden action exit2l29() {
        c = 32w5;
    }
    @hidden table tbl_exit2l19 {
        actions = {
            exit2l19();
        }
        const default_action = exit2l19();
    }
    @hidden table tbl_e {
        actions = {
            e();
        }
        const default_action = e();
    }
    @hidden table tbl_exit2l29 {
        actions = {
            exit2l29();
        }
        const default_action = exit2l29();
    }
    apply {
        tbl_exit2l19.apply();
        tbl_e.apply();
        if (hasExited) {
            ;
        } else {
            tbl_exit2l29.apply();
        }
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;
