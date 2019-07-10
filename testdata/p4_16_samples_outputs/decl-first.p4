control p(in bit<1> y_0) {
    apply {
        bit<1> x;
        x = 1w0;
        if (x == 1w0) {
            bit<1> y;
            y = 1w1;
        } else {
            bit<1> y;
            y = y_0;
            {
                bit<1> y;
                y = 1w0;
                {
                    bit<1> y;
                    y = 1w0;
                }
            }
        }
    }
}

