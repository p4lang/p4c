extern E {
    E();
    bit<8> get();
    void set(in bit<8> value);
    abstract bool f();
}

E() e = {
    bool f() {
        bit<8> x = this.get() + 1;
        if (x >= 4) {
            return true;
        } else {
            this.set(x);
        }
        return false;
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
