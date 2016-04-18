control p(inout bit bt) {
    action a(inout bit y0)
    { y0 = y0 | 1w1; }
    
    action b() {
        a(bt);
        a(bt);
    }

    table t() {
        actions = { b; }
        default_action = b;
    }
    
    apply {
        t.apply();
    }
}

package m(p pipe);

m(p()) main;
