parser p() {
    state start {
        transition accept;
    }
}

parser q() {
    state start {
        p.apply();
        p.apply();
        transition accept;
    }
}

