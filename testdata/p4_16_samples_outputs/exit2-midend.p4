control ctrl(out bit<32> c) {
    bool hasExited;
    @name("ctrl.e") action e() {
        hasExited = true;
    }
    @hidden action exit2l22() {
        hasExited = false;
        c = 32w2;
    }
    @hidden action exit2l32() {
        c = 32w5;
    }
    @hidden table tbl_exit2l22 {
        actions = {
            exit2l22();
        }
        const default_action = exit2l22();
    }
    @hidden table tbl_e {
        actions = {
            e();
        }
        const default_action = e();
    }
    @hidden table tbl_exit2l32 {
        actions = {
            exit2l32();
        }
        const default_action = exit2l32();
    }
    apply {
        tbl_exit2l22.apply();
        tbl_e.apply();
        if (hasExited) {
            ;
        } else {
            tbl_exit2l32.apply();
        }
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;
