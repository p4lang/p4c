parser mypt(in bit<1> tt);
package mypack(mypt t);
parser MyParser(in bit<1> tt) {
    state start {
        transition select(tt) {
            0: accept;
            default: reject;
        }
    }
}

parser MyParser1(in bit<1> tt) {
    mypt t = MyParser();
    state start {
        t.apply(tt);
        transition select(tt) {
            0: accept;
            default: reject;
        }
    }
}

parser MyParser2(in bit<1> tt) {
    mypt t;
    state start {
        t = MyParser();
        t.apply(tt);
        transition select(tt) {
            0: accept;
            default: reject;
        }
    }
}

mypack(MyParser1()) main;
