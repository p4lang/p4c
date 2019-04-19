struct intrinsic_metadata_t {
    bit<8> f0;
    bit<8> f1;
}

struct empty_t {
}

control C<H, M>(inout H hdr, inout M meta, in intrinsic_metadata_t intr_md={ 0, 0 });
package P<H, M>(C<H, M> c);
struct hdr_t {
}

struct meta_t {
}

control MyC(inout hdr_t hdr, inout meta_t meta) {
    apply {
    }
}

P<hdr_t, meta_t>(MyC()) main;

