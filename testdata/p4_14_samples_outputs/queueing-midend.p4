#include <core.p4>
#include <v1model.p4>

@name("queueing_metadata_t") struct queueing_metadata_t_0 {
    bit<48> enq_timestamp;
    bit<24> enq_qdepth;
    bit<32> deq_timedelta;
    bit<24> deq_qdepth;
}

header hdr1_t {
    bit<8> f1;
    bit<8> f2;
}

header queueing_metadata_t {
    bit<48> enq_timestamp;
    bit<24> enq_qdepth;
    bit<32> deq_timedelta;
    bit<24> deq_qdepth;
}

struct __metadataImpl {
    @name("queueing_metadata") 
    queueing_metadata_t_0 queueing_metadata;
    @name("standard_metadata") 
    standard_metadata_t   standard_metadata;
}

struct __headersImpl {
    @name("hdr1") 
    hdr1_t              hdr1;
    @name("queueing_hdr") 
    queueing_metadata_t queueing_hdr;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".queueing_dummy") state queueing_dummy {
        packet.extract<queueing_metadata_t>(hdr.queueing_hdr);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<hdr1_t>(hdr.hdr1);
        transition select(meta.standard_metadata.packet_length) {
            32w0: queueing_dummy;
            default: accept;
        }
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name(".set_port") action set_port_0(bit<9> port) {
        meta.standard_metadata.egress_spec = port;
    }
    @name("._drop") action _drop_0() {
        mark_to_drop();
    }
    @name(".t_ingress") table t_ingress {
        actions = {
            set_port_0();
            _drop_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.hdr1.f1: exact @name("hdr.hdr1.f1") ;
        }
        size = 128;
        default_action = NoAction_0();
    }
    apply {
        t_ingress.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<hdr1_t>(hdr.hdr1);
        packet.emit<queueing_metadata_t>(hdr.queueing_hdr);
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
