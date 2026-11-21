parser Parser1() {
    @name("Parser1.l") bit<8> l_0;
    state start {
        l_0 = 8w0;
        transition select(l_0) {
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
