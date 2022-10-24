#include <core.p4>

header data_t {
    bit<16> h1;
}

struct headers {
    data_t data;
}

control ingress(inout headers hdr) {
    @hidden action issue2492l13() {
        hdr.data.h1 = 16w1;
    }
    @hidden table tbl_issue2492l13 {
        actions = {
            issue2492l13();
        }
        const default_action = issue2492l13();
    }
    apply {
        tbl_issue2492l13.apply();
    }
}

control ctr<H>(inout H hdr);
package top<H1, H2, H3>(ctr<H1> ctrl1, @optional ctr<H2> ctrl2, @optional ctr<H3> ctrl3);
top<headers, _, _>(ingress()) main;
