control strength() {
    apply {
        bit<4> x;
        x = x & 4w0;
        x = 4w0 & x;
        x = x | 4w0;
        x = 4w0 | x;
        x = 4w0 ^ x;
        x = x ^ 4w0;
        x = x << 4w0;
        x = x >> 4w0;
        bit<4> y;
        y = y + 4w0;
        y = 4w0 + y;
        y = y - 4w0;
        y = 4w0 - y;
        y = y * 4w0;
        y = y * 4w1;
        y = 4w1 * y;
        y = 4w0 * y;
        y = y * 2;
        y = y * 16;
        y = 8 * y;
        y = y / 2;
        y = y / 8;
        y = y / 4w1;
        y = 4w0 / y;
        y = y % 4w1;
        y = y % 4;
        y = y - 4w1;
        y = y - 4w0xf;
        int<4> w;
        w = w - 4s1;
        w = w - -4s0x1;
        bool z;
        z = z && true;
        z = true && z;
        z = z && false;
        z = false && z;
        z = z || true;
        z = true || z;
        z = z || false;
        z = false || z;
    }
}

