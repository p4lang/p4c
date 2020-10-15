control c(inout bit<8> v) {
    apply {
        @name("c.hasReturned_0") bool hasReturned = false;
        {
            @name("c.val_0") bit<8> val_0 = v;
            @name("c.hasReturned") bool hasReturned_0 = false;
            if (val_0 == 8w0) {
                val_0 = 8w1;
                hasReturned_0 = true;
            }
            if (!hasReturned_0) {
                val_0 = 8w2;
            }
            v = val_0;
        }
        if (v == 8w0) {
            v = 8w1;
            hasReturned = true;
        }
        if (!hasReturned) {
            v = 8w2;
        }
    }
}

control e(inout bit<8> _v);
package top(e _e);
top(c()) main;

