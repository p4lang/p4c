control c(inout bit<8> v) {
    @name("c.hasReturned_0") bool hasReturned;
    @name("c.val_0") bit<8> val;
    @name("c.hasReturned") bool hasReturned_0;
    apply {
        hasReturned = false;
        val = v;
        hasReturned_0 = false;
        if (val == 8w0) {
            val = 8w1;
            hasReturned_0 = true;
        }
        if (hasReturned_0) {
            ;
        } else {
            val = 8w2;
        }
        v = val;
        if (v == 8w0) {
            v = 8w1;
            hasReturned = true;
        }
        if (hasReturned) {
            ;
        } else {
            v = 8w2;
        }
    }
}

control e(inout bit<8> _v);
package top(e _e);
top(c()) main;
