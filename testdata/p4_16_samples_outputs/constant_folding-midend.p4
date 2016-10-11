control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    bool w;
    bit<8> z;
    action act() {
        x = 32w8;
        x = 32w8;
        x = 32w8;
        x = 32w8;
        x = 32w2;
        x = 32w2;
        x = 32w2;
        x = 32w2;
        x = 32w15;
        x = 32w15;
        x = 32w1;
        x = 32w1;
        x = 32w2;
        x = 32w1;
        x = 32w1;
        x = 32w1;
        x = 32w7;
        x = 32w7;
        x = 32w6;
        x = 32w6;
        x = 32w40;
        x = 32w40;
        x = 32w2;
        x = 32w2;
        x = 32w5;
        x = 32w5;
        x = 32w17;
        w = false;
        w = false;
        w = true;
        w = true;
        w = false;
        w = false;
        w = true;
        w = true;
        w = false;
        w = false;
        w = true;
        w = true;
        w = false;
        w = true;
        w = true;
        w = false;
        z = 8w0;
        z = 8w0;
        z = 8w128;
        z = 8w128;
        z = 8w0;
        z = 8w0;
        z = 8w0;
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

top(c()) main;
