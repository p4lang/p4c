#include <core.p4>
#include <v1model.p4>

control p();
package top(p _p);
control c() {
    bit<16> var;
    bit<32> hdr_1;
    apply {
        hdr_1 = 32w0;
        hash<bit<16>, bit<16>, bit<32>, bit<16>>(var, HashAlgorithm.crc16, 16w0, hdr_1, 16w0xffff);
    }
}

top(c()) main;

