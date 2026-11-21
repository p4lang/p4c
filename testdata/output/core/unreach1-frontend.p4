#include <core.p4>

header h1_t {
    bit<4>[4] a;
    bit<16>   b;
}

header h2_t {
    bit<16> x;
    bit<16> y;
}

struct headers_t {
    h1_t    h1;
    h2_t[4] h2;
}

struct empty_metadata_t {
}

control c(inout headers_t headers, inout empty_metadata_t user_meta) {
    @name("c.i") bit<16> i_0;
    apply {
        for (@name("c.i") bit<16> i_0 in 16w0 .. 16w3) {
            switch (headers.h1.a[i_0]) {
                4w0: 
                4w1: 
                4w2: {
                    if (headers.h1.b > 16w100) {
                        headers.h2[i_0].x = headers.h1.b;
                        break;
                    } else {
                        headers.h2[i_0].y = headers.h1.b;
                        continue;
                        headers.h2[i_0].setInvalid();
                    }
                }
                4w4: {
                    headers.h2[i_0].x = headers.h1.b;
                    break;
                    headers.h2[i_0].setInvalid();
                }
            }
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top<headers_t, empty_metadata_t>(c()) main;
