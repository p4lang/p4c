extern E {
    E();
    bit<8> get();
    void set(in bit<8> value);
    abstract bool f();
}

E() e = {
    bool f() {
        @name("tmp") bit<8> tmp;
        tmp = this.get();
        if (tmp + 8w1 >= 8w4) {
            return true;
        } else {
            this.set(tmp + 8w1);
            return false;
        }
    }
};
control C();
control Impl() {
    @hidden action issue5445l24() {
        e.f();
    }
    @hidden table tbl_issue5445l24 {
        actions = {
            issue5445l24();
        }
        const default_action = issue5445l24();
    }
    apply {
        tbl_issue5445l24.apply();
    }
}

package P(C c);
P(Impl()) main;
