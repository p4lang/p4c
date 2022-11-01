#include <core.p4>

struct intrinsic_metadata_t {
    bit<8> f0;
    bit<8> f1;
}

struct empty_t {
}

control C<H, M>(inout H hdr, inout M meta, in intrinsic_metadata_t intr_md);
package P<H, M>(C<H, M> c);
struct hdr_t {
}

struct meta_t {
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
}

control MyC(inout hdr_t hdr, inout meta_t meta, in intrinsic_metadata_t intr_md) {
    bit<8> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyC.c2.a") table c2_a {
        key = {
            key_0: exact @name("meta.f0");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action issue1638l23() {
        key_0 = 8w0;
    }
    @hidden table tbl_issue1638l23 {
        actions = {
            issue1638l23();
        }
        const default_action = issue1638l23();
    }
    apply {
        tbl_issue1638l23.apply();
        c2_a.apply();
    }
}

P<hdr_t, meta_t>(MyC()) main;
