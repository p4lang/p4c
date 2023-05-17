control ComputeChecksum<H>(inout H hdr);
extern void update_checksum_with_payload<T, O>(in T data, out O ck);
struct flags {
    bit<1> cwr;
}

header tcp {
    bit<1>  _flags_cwr0;
    bit<16> _checksum1;
}

struct headers {
    tcp tcp;
}

control computeChecksum(inout headers hdr) {
    @hidden action issue2982l20() {
        update_checksum_with_payload<flags, bit<16>>((flags){cwr = hdr.tcp._flags_cwr0}, hdr.tcp._checksum1);
    }
    @hidden table tbl_issue2982l20 {
        actions = {
            issue2982l20();
        }
        const default_action = issue2982l20();
    }
    apply {
        tbl_issue2982l20.apply();
    }
}

package top(ComputeChecksum<headers> _c);
top(computeChecksum()) main;
