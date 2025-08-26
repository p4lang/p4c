struct S {
}

control C() {
    apply {
        S s = (false ? { ... } : true);
    }
}

