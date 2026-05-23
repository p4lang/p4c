control c(inout bit<32> x) {
    @hidden action inlinefunction17() {
        x = x + x;
    }
    @hidden table tbl_inlinefunction17 {
        actions = {
            inlinefunction17();
        }
        const default_action = inlinefunction17();
    }
    apply {
        tbl_inlinefunction17.apply();
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;
