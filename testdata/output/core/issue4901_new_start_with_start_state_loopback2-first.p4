parser Parser1() {
    bit<8> l = 8w0;
    state start {
        transition next;
    }
    state next {
        transition select(l) {
            8w2: start;
            default: accept;
        }
    }
}

parser Parser2() {
    bit<7> l = 7w1;
    state start {
        transition select(l) {
            7w0: s0;
            default: accept;
        }
    }
    state s0 {
        transition accept;
    }
}

parser proto();
package top(proto p1, proto p2);
top(Parser1(), Parser2()) main;
