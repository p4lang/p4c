control c(inout bit<8> v) {
    @name("c.val_0") bit<8> val;
    apply {
        val = v;
        if (val == 8w0) {
            val = 8w1;
        } else {
            val = 8w2;
        }
        v = val;
        if (v == 8w0) {
            v = 8w1;
        } else {
            v = 8w2;
        }
    }
}

control e(inout bit<8> _v);
package top(e _e);
top(c()) main;
