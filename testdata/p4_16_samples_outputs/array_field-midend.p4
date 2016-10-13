header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> a;
    bit<32> tmp_10;
    bit<32> tmp_11;
    bit<32> tmp_12;
    bit<32> tmp_13;
    bit<1> tmp_14;
    bit<1> tmp_15;
    bit<1> tmp_16;
    bit<32> tmp_17;
    bit<1> tmp_18;
    bit<1> tmp_19;
    bit<1> tmp_20;
    action act() {
        a = 32w0;
        tmp_10 = a;
        s[tmp_10].z = 1w1;
        tmp_11 = a + 32w1;
        tmp_12 = tmp_11;
        s[tmp_12].z = 1w0;
        tmp_13 = a;
        tmp_14 = s[tmp_13].z;
        tmp_15 = 1w0;
        tmp_16 = f(tmp_14, tmp_15);
        s[tmp_13].z = tmp_14;
        a = (bit<32>)tmp_16;
        tmp_17 = a;
        tmp_18 = s[tmp_17].z;
        tmp_19 = 1w1;
        tmp_20 = f(tmp_18, tmp_19);
        s[tmp_17].z = tmp_18;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

top(my()) main;
