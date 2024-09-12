parser Parser1() {
    @name("Parser1.l") bit<8> l_0;
    state start {
        l_0 = 8w0;
        transition start_0;
    }
    state start_0 {
        transition select(l_0) {
            8w2: start_0;
            default: accept;
        }
    }
}

parser Parser2() {
    @name("Parser2.l") bit<7> l_1;
    state start {
        l_1 = 7w1;
        transition select(l_1) {
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
