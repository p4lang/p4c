control c(inout bit<32> x) {
    bit<32> tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    bool tmp_2;
    apply {
        x = 32w10;
        tmp_2 = x == 32w10;
        if (tmp_2) {
            tmp = x + 32w2;
            x = tmp;
            tmp_0 = x + 32w4294967290;
            x = tmp_0;
        }
        else {
            tmp_1 = x << 2;
            x = tmp_1;
        }
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;
