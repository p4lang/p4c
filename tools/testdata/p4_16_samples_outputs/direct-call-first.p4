control c() {
    apply {
    }
}

control d() {
    @name("c") c() c_inst;
    apply {
        c_inst.apply();
    }
}

