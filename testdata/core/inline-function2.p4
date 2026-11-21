bit foo(in bit a) {
  return a + 1; 
}

control p(inout bit bt) {
    action a(inout bit y0, bit y1) {
      bit y2 = y1 > 0 ? 1w1 : 0;
      if (y2 == 1) {
         y0 = 0;
      } else if (y1 != 1) {
         y0 = y0 | 1w1;
      }
    }

    action b() {
        a(bt, foo(bt));
        a(bt, 1);
    }

    table t {
        actions = { b; }
        default_action = b;
    }

    apply {
        t.apply();
    }
}

control simple<T>(inout T arg);
package m<T>(simple<T> pipe);

m(p()) main;
