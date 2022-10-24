control c(inout bit<32> x) {
    @hidden action synthaction19() {
        x = 32w10;
        x = 32w12;
        x = 32w6;
    }
    @hidden table tbl_synthaction19 {
        actions = {
            synthaction19();
        }
        const default_action = synthaction19();
    }
    apply {
        tbl_synthaction19.apply();
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;
