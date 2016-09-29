parser Parser();
package Package(Parser p1, Parser p2);
parser Parser1()(Parser p) {
    state start {
        p.apply();
        transition accept;
    }
}

parser Parser2()(Parser p) {
    state start {
        p.apply();
        transition accept;
    }
}

parser Inside() {
    state start {
        transition accept;
    }
}

Parser1(Inside()) p1;
Parser2(Inside()) p2;
Package(p1, p2) main;
