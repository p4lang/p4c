#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

header extra_t {
    bit<16> h;
    bit<8>  b1;
    bit<8>  b2;
}

struct __metadataImpl {
}

struct __headersImpl {
    @name("data") 
    data_t     data;
    @name(".extra") 
    extra_t[4] extra;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".extra") state extra {
        packet.extract<extra_t>(hdr.extra.next);
        transition select(hdr.extra.last.b2) {
            8w0x80 &&& 8w0x80: extra;
            default: accept;
        }
    }
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition extra;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".set0b1") action set0b1(bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name(".act1") action act1(bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name(".act2") action act2(bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name(".act3") action act3(bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name(".noop") action noop() {
    }
    @name(".setb2") action setb2(bit<8> val) {
        hdr.data.b2 = val;
    }
    @name(".set1b1") action set1b1(bit<8> val) {
        hdr.extra[1].b1 = val;
    }
    @name(".set2b2") action set2b2(bit<8> val) {
        hdr.extra[2].b2 = val;
    }
    @name(".setb1") action setb1(bit<9> port, bit<8> val) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".ex1") table ex1 {
        actions = {
            set0b1();
            act1();
            act2();
            act3();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.extra[0].h: ternary @name("hdr..extra[0].h") ;
        }
        default_action = NoAction();
    }
    @name(".tbl1") table tbl1 {
        actions = {
            setb2();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2: ternary @name("hdr.data.f2") ;
        }
        default_action = NoAction();
    }
    @name(".tbl2") table tbl2 {
        actions = {
            set1b1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2: ternary @name("hdr.data.f2") ;
        }
        default_action = NoAction();
    }
    @name(".tbl3") table tbl3 {
        actions = {
            set2b2();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2: ternary @name("hdr.data.f2") ;
        }
        default_action = NoAction();
    }
    @name(".test1") table test1 {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: ternary @name("hdr.data.f1") ;
        }
        default_action = NoAction();
    }
    apply {
        test1.apply();
        switch (ex1.apply().action_run) {
            act1: {
                tbl1.apply();
            }
            act2: {
                tbl2.apply();
            }
            act3: {
                tbl3.apply();
            }
        }

    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
        packet.emit<extra_t[4]>(hdr.extra);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
