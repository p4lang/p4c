control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    apply {
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
        bool w;
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
        bit<8> z;
        z = 8w0;
        z = 8w0;
        z = 8w128;
        z = 8w128;
        z = 8w0;
        z = 8w0;
        z = 8w0;
    }
}

top(c()) main;

