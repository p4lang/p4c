header header_h {
    bit<8> field;
}

struct struct_t {
    header_h hdr;
}

control ctrl(inout struct_t input, out bit<8> out1, out header_h out2) {
    bit<8> tmp0_0;
    bit<8> tmp1_0;
    header_h tmp2_0;
    header_h tmp3_0;
    @name("ctrl.act") action act() {
        tmp0_0 = input.hdr.field;
        input.hdr.setValid();
        tmp1_0 = tmp0_0;
        tmp2_0 = input.hdr;
        input.hdr.setInvalid();
        tmp3_0 = tmp2_0;
    }
    apply {
        act();
        out1 = tmp1_0;
        out2 = tmp3_0;
    }
}

control MyControl<S, H>(inout S i, out bit<8> o1, out H o2);
package MyPackage<S, H>(MyControl<S, H> c);
MyPackage<struct_t, header_h>(ctrl()) main;

