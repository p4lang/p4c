parser Parser();
parser MyParser(Parser p);
package Package(MyParser p1, MyParser p2);
parser Parser1(Parser p) {
    state start {
        p.apply();
        transition accept;
    }
}

parser Parser2(Parser p) {
    state start {
        p.apply();
        transition accept;
    }
}

Parser1() p1;

Parser2() p2;

Package(p1, p2) main;

