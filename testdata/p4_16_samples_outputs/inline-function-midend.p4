control c(inout bit<32> x) {
    @name("c.tmp") bit<32> tmp_2;
    @hidden action inlinefunction2() {
        tmp_2 = x;
    }
    @hidden action inlinefunction2_0() {
        tmp_2 = x;
    }
    @hidden action inlinefunction11() {
        x = x + tmp_2;
    }
    @hidden table tbl_inlinefunction2 {
        actions = {
            inlinefunction2();
        }
        const default_action = inlinefunction2();
    }
    @hidden table tbl_inlinefunction2_0 {
        actions = {
            inlinefunction2_0();
        }
        const default_action = inlinefunction2_0();
    }
    @hidden table tbl_inlinefunction11 {
        actions = {
            inlinefunction11();
        }
        const default_action = inlinefunction11();
    }
    apply {
        if (x > x) {
            tbl_inlinefunction2.apply();
        } else {
            tbl_inlinefunction2_0.apply();
        }
        tbl_inlinefunction11.apply();
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;
