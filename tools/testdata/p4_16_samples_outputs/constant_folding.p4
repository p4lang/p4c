control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    apply {
        x = 5 + 3;
        x = 32w5 + 3;
        x = 32w5 + 32w3;
        x = 5 + 32w3;
        x = 5 - 3;
        x = 32w5 - 3;
        x = 32w5 - 32w3;
        x = 5 - 32w3;
        x = 5 * 3;
        x = 32w5 * 3;
        x = 5 / 3;
        x = 32w5 / 3;
        x = 5 % 3;
        x = 32w5 / 3;
        x = 5 & 3;
        x = 32w5 & 3;
        x = 5 | 3;
        x = 32w5 | 3;
        x = 5 ^ 3;
        x = 32w5 ^ 3;
        x = 5 << 3;
        x = 32w5 << 3;
        x = 5 >> 1;
        x = 32w5 >> 1;
        x = 5 << 0;
        x = 5 >> 0;
        x = (bit<32>)(4w1 ++ 4w1);
        bool w;
        w = 5 == 3;
        w = 32w5 == 3;
        w = 5 != 3;
        w = 32w5 != 3;
        w = 5 < 3;
        w = 32w5 < 3;
        w = 5 > 3;
        w = 32w5 > 3;
        w = 5 <= 3;
        w = 32w5 <= 3;
        w = 5 >= 3;
        w = 32w5 >= 3;
        w = true == false;
        w = true != false;
        w = true == true;
        w = true != true;
        bit<8> z;
        z = 128 + 128;
        z = 8w128 + 128;
        z = 0 - 128;
        z = 8w0 - 128;
        z = 1 << 9;
        z = 8w1 << 9;
        z = 10 >> 9;
    }
}

top(c()) main;

