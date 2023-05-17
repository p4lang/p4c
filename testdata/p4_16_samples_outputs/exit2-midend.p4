control ctrl(out bit<32> c) {
    bool hasExited;
    @name("ctrl.e") action e() {
        hasExited = true;
    }
    @hidden action exit2l31() {
        hasExited = false;
        c = 32w2;
    }
    @hidden action exit2l41() {
        c = 32w5;
    }
    @hidden table tbl_exit2l31 {
        actions = {
            exit2l31();
        }
        const default_action = exit2l31();
    }
    @hidden table tbl_e {
        actions = {
            e();
        }
        const default_action = e();
    }
    @hidden table tbl_exit2l41 {
        actions = {
            exit2l41();
        }
        const default_action = exit2l41();
    }
    apply {
        tbl_exit2l31.apply();
        tbl_e.apply();
        if (hasExited) {
            ;
        } else {
            tbl_exit2l41.apply();
        }
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;
