void do_something(inout bit<8> val) {
    if (val == 8w0) {
        val = 8w1;
        return;
    }
    val = 8w2;
}
control c(inout bit<8> v) {
    apply {
        do_something(v);
        if (v == 8w0) {
            v = 8w1;
            return;
        }
        v = 8w2;
    }
}

control e(inout bit<8> _v);
package top(e _e);
top(c()) main;
