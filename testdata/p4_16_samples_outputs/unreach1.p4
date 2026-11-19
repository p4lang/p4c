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
    apply {
        for (bit<16> i in 0 .. 3) {
            switch (headers.h1.a[i]) {
                0: 
                1: 
                2: {
                    if (headers.h1.b > 100) {
                        headers.h2[i].x = headers.h1.b;
                        break;
                    } else {
                        headers.h2[i].y = headers.h1.b;
                        continue;
                    }
                    headers.h2[i].setInvalid();
                }
                4: {
                    headers.h2[i].x = headers.h1.b;
                    break;
                    headers.h2[i].setInvalid();
                }
            }
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top(c()) main;
