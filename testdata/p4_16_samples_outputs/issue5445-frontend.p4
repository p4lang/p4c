extern E {
    E();
    bit<8> get();
    void set(in bit<8> value);
    abstract bool f();
}

E() e = {
    bool f() {
        @name("tmp") bit<8> tmp;
        @name("tmp_0") bit<8> tmp_0;
        @name("x") bit<8> x_0;
        tmp = this.get();
        tmp_0 = tmp + 8w1;
        x_0 = tmp_0;
        if (x_0 >= 8w4) {
            return true;
        } else {
            this.set(x_0);
            return false;
        }
    }
};
control C();
control Impl() {
    apply {
        e.f();
    }
}

package P(C c);
P(Impl()) main;
