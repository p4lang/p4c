control c(inout bit<32> x) {
    @hidden action synthaction10() {
        x = 32w10;
        x = 32w12;
        x = 32w6;
    }
    @hidden table tbl_synthaction10 {
        actions = {
            synthaction10();
        }
        const default_action = synthaction10();
    }
    apply {
        tbl_synthaction10.apply();
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;
