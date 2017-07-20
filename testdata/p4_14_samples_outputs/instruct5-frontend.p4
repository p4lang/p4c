#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  more;
}

@name("data2_t") header data2_t_0 {
    bit<24> x1;
    bit<8>  more;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("data") 
    data_t       data;
    @name(".extra") 
    data2_t_0[4] extra;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".parse_extra") state parse_extra {
        packet.extract<data2_t_0>(hdr.extra.next);
        transition select(hdr.extra.last.more) {
            8w0: accept;
            default: parse_extra;
        }
    }
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition select(hdr.data.more) {
            8w0: accept;
            default: parse_extra;
        }
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".output") action output_1(bit<9> port) {
        meta.standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop_0() {
    }
    @name(".push1") action push1_0(bit<24> x1) {
        hdr.extra.push_front(1);
        hdr.extra[0].x1 = x1;
        hdr.extra[0].more = hdr.data.more;
        hdr.data.more = 8w1;
    }
    @name(".push2") action push2_0(bit<24> x1, bit<24> x2) {
        hdr.extra.push_front(2);
        hdr.extra[0].x1 = x1;
        hdr.extra[0].more = 8w1;
        hdr.extra[1].x1 = x2;
        hdr.extra[1].more = hdr.data.more;
        hdr.data.more = 8w1;
    }
    @name(".pop1") action pop1_0() {
        hdr.data.more = hdr.extra[0].more;
        hdr.extra.pop_front(1);
    }
    @name(".output") table output_2 {
        actions = {
            output_1();
        }
        default_action = output_1(9w1);
    }
    @name(".test1") table test1_0 {
        actions = {
            noop_0();
            push1_0();
            push2_0();
            pop1_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: exact @name("hdr.data.f1") ;
        }
        default_action = NoAction();
    }
    apply {
        test1_0.apply();
        output_2.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
        packet.emit<data2_t_0[4]>(hdr.extra);
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
