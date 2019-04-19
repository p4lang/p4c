parser p() {
    state start {
        transition accept;
    }
}

parser q() {
    state start {
        p.apply();
        transition accept;
    }
}

