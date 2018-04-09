#include <core.p4>
#include <v1model.p4>

typedef bit<16> Hash;
control p();
package top(p _p);
control c() {
    apply {
        bit<16> var;
        bit<32> hdr = 32w0;
        hash<bit<16>, bit<16>, bit<32>, bit<16>>(var, HashAlgorithm.crc16, 16w0, hdr, 16w0xffff);
    }
}

top(c()) main;

