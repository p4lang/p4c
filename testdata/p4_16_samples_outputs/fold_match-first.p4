parser p() {
    state start {
        transition accept;
    }
    state next0 {
        transition accept;
    }
    state next1 {
        transition accept;
    }
    state next2 {
        transition accept;
    }
    state next3 {
        transition accept;
    }
    state next00 {
        transition accept;
    }
    state next01 {
        transition accept;
    }
    state next02 {
        transition accept;
    }
    state next03 {
        transition accept;
    }
    state last {
        transition select(32w0) {
        }
    }
}

