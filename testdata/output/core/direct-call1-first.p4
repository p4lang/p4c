parser p() {
    state start {
        transition accept;
    }
}

parser q() {
    @name("p") p() p_inst;
    state start {
        p_inst.apply();
        transition accept;
    }
}

