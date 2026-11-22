#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

struct headers_t {
    ipv4_t ipv4;
}

control c(inout headers_t headers) {
    apply {
        switch (headers.ipv4.src_addr[15:0]) {
            1: {
                for (bit<32> i33 = 10; i33 != 0; i33 -= 1) {
                    switch (i33) {
                        0: {
                            for (bit<32> j34 = 10; j34 != 0; j34 -= 1) {
                                headers.ipv4.src_addr = 1;
                            }
                        }
                        1: {
                            for (bit<32> j35 = 10; j35 != 0; j35 -= 1) {
                                headers.ipv4.src_addr = 2;
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
            default: {
            }
        }
    }
}

top(c()) main;
