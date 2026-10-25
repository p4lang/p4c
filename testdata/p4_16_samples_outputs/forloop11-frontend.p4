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
    @name("c.i33") bit<32> i33_0;
    @name("c.j34") bit<32> j34_0;
    @name("c.j35") bit<32> j35_0;
    apply {
        switch (headers.ipv4.src_addr[15:0]) {
            16w1: {
                for (i33_0 = 32w10; i33_0 != 32w0; i33_0 = i33_0 - 32w1) {
                    switch (i33_0) {
                        32w0: {
                            for (j34_0 = 32w10; j34_0 != 32w0; j34_0 = j34_0 - 32w1) {
                                headers.ipv4.src_addr = 32w1;
                            }
                        }
                        32w1: {
                            for (j35_0 = 32w10; j35_0 != 32w0; j35_0 = j35_0 - 32w1) {
                                headers.ipv4.src_addr = 32w2;
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

top<headers_t>(c()) main;
