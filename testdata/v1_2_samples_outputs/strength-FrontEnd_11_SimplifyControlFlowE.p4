control strength() {
    apply {
        bit<4> x;
        x = 4w0;
        x = 4w0;
        x = x;
        x = x;
        x = x;
        x = x;
        x = x;
        x = x;
        bit<4> y;
        y = y;
        y = y;
        y = y;
        y = -y;
        y = 4w0;
        y = y;
        y = y;
        y = 4w0;
        y = y << 1;
        y = 4w0;
        y = y << 3;
        y = y >> 1;
        y = y >> 3;
        y = y;
        y = 4w0;
        y = y & 4w0;
        y = y & 4w3;
        y = y + 4w15;
        y = y + 4w0x1;
        int<4> w;
        w = w + -4s1;
        w = w + 4s0x1;
        bool z;
        z = z;
        z = z;
        z = z && false;
        z = false;
        z = z || true;
        z = true;
        z = z;
        z = z;
    }
}

