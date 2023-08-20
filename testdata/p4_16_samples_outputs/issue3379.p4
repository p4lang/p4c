parser mypt<t>(in t tt);
parser MyParser1(in bit<1> tt) {
    state start {
        transition select(tt) {
            0: accept;
            default: reject;
        }
    }
}

typedef bit<1> t;
package mypackaget(mypt<t> t2);
package mypackaget(mypt<t> t1, mypt<t> t2);
mypackaget(MyParser1()) t4;
