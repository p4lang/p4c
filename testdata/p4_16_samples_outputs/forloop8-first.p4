@test_keep_opassign header ipv4_t {
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

struct empty_metadata_t {
}

control c(inout headers_t headers, inout empty_metadata_t user_meta) {
    apply {
        bit<32> sum = 32w1;
        @unroll for (bit<8> i = 8w20; i != 8w0; i -= 8w2) {
            if (i == 8w64) {
                continue;
            } else {
                sum += 32w1;
                i -= 8w1;
            }
        }
        if (sum == 32w10) {
            headers.ipv4.src_addr = sum;
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top<headers_t, empty_metadata_t>(c()) main;
