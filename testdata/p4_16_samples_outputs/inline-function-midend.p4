control c(inout bit<32> x) {
    @hidden action inlinefunction11() {
        x = x + x;
    }
    @hidden table tbl_inlinefunction11 {
        actions = {
            inlinefunction11();
        }
        const default_action = inlinefunction11();
    }
    apply {
        tbl_inlinefunction11.apply();
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;
