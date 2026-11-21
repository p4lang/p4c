parser Parser1() {
    state start {
        transition accept;
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
