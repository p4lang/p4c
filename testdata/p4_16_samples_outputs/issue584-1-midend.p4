#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control p();
package top(p _p);
control c() {
    @name("c.var") bit<16> var_0;
    @hidden action issue5841l28() {
        hash<bit<16>, bit<16>, bit<32>, bit<16>>(var_0, HashAlgorithm.crc16, 16w0, 32w0, 16w0xffff);
    }
    @hidden table tbl_issue5841l28 {
        actions = {
            issue5841l28();
        }
        const default_action = issue5841l28();
    }
    apply {
        tbl_issue5841l28.apply();
    }
}

top(c()) main;
