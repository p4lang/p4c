parser p() {
    state start {
        transition accept;
    }
}

parser q() {
    @name("p") p() p_inst;
    @name("p") p() p_inst_0;
    state start {
        p_inst.apply();
        p_inst_0.apply();
        transition accept;
    }
}

