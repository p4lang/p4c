#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<16> Hash;
control p();
package top(p _p);
control c() {
    @name("c.var") bit<16> var_0;
    @name("c.hdr") bit<32> hdr_0;
    apply {
        hdr_0 = 32w0;
        hash<bit<16>, bit<16>, bit<32>, bit<16>>(var_0, HashAlgorithm.crc16, (Hash)0, hdr_0, (Hash)0xffff);
    }
}

top(c()) main;

