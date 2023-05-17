parser mypt<t>(in t tt);

parser MyParser1(in bit tt) {
  state start {
    transition select(tt) {
     0: accept;
     _: reject;
   }
 }
}

typedef bit t;
package mypackaget(mypt<t> t2);
package mypackaget(mypt<t> t1, mypt<t> t2);
mypackaget(MyParser1()) t4;
