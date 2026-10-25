bit foo(in bit a) {
  return a + 1; 
}

control p(inout bit bt, in bit bt2, in bit bt3) {
    action a(inout bit y0, bit y1, bit y2) {
      bit y3 = y1 > 0 ? 1w1 : 0;
      if (y3 == 1) {
         y0 = 0;
      } else if (y0 != 1) {
         y0 = y2 | y1;
      }
    }

    action b() {
        a(bt, foo(bt2), foo(bt3));
    }

    table t {
        actions = { b; }
        default_action = b;
    }

    apply {
        t.apply();
    }
}

control simple<T>(inout T arg, in T brg, in T crg);
package m<T>(simple<T> pipe);

m(p()) main;
