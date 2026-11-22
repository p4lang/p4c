parser Parser1() {
    bit<8> l = 0;
    state start {
        transition next;
    }
    state next {
        transition select(l) {
            default: accept;
        }
    }
}

parser Parser2() {
    state start {
        transition accept;
    }
}

parser proto();
package top(proto p1, proto p2);
top(Parser1(), Parser2()) main;
