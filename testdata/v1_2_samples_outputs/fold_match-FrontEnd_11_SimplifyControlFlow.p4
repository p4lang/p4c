parser p() {
    state start {
        transition select(32w0) {
            32w5 &&& 32w7: reject;
            32w0: accept;
            default: reject;
        }
    }
    state next0 {
        transition select(32w0) {
            32w1 &&& 32w1: reject;
            default: accept;
            32w0: reject;
        }
    }
    state next1 {
        transition select(32w1) {
            32w1 &&& 32w1: accept;
            32w0: reject;
            default: reject;
        }
    }
    state next2 {
        transition select(32w1) {
            32w0 .. 32w7: accept;
            32w0: reject;
            default: reject;
        }
    }
    state next3 {
        transition select(32w3) {
            32w1 &&& 32w1: accept;
            32w0: reject;
            default: reject;
        }
    }
    state next00 {
        transition select(true, 32w0) {
            (true, 32w1 &&& 32w1): reject;
            default: accept;
            (true, 32w0): reject;
        }
    }
    state next01 {
        transition select(true, 32w1) {
            (true, 32w1 &&& 32w1): accept;
            (true, 32w0): reject;
            default: reject;
        }
    }
    state next02 {
        transition select(true, 32w1) {
            (true, 32w0 .. 32w7): accept;
            (true, 32w0): reject;
            default: reject;
        }
    }
    state next03 {
        transition select(true, 32w3) {
            (true, 32w1 &&& 32w1): accept;
            (true, 32w0): reject;
            default: reject;
        }
    }
    state last {
        transition select(32w0) {
            32w5 &&& 32w7: reject;
            32w1: reject;
        }
    }
}

