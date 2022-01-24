#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<16> Hash;
control p();
package top(p _p);
control c() {
    apply {
        bit<16> var;
        bit<32> hdr = 32w0;
        hash<bit<16>, bit<16>, bit<32>, bit<16>>(var, HashAlgorithm.crc16, (Hash)0, hdr, (Hash)0xffff);
    }
}

top(c()) main;

