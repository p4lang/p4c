parser p() {
    state start {
    }
}

parser q() {
    state start {
        p.apply(_);
    }
}

