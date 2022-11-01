parser mypt<t>(in t tt);
parser MyParser1(in bit<1> tt) {
    state start {
        transition select(tt) {
            1w0: accept;
            default: reject;
        }
    }
}

package mypackaget<t>(mypt<t> t2);
package mypackaget<t>(mypt<t> t1, mypt<t> t2);
mypackaget<bit<1>>(MyParser1()) t4;
