#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> x;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("data") 
    data_t data;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".my_drop") action my_drop_0() {
        mark_to_drop();
    }
    @name(".set_egress_port") action set_egress_port_0(bit<9> egress_port) {
        meta.standard_metadata.egress_spec = egress_port;
    }
    @name(".repeater") table repeater_0 {
        actions = {
            my_drop_0();
            set_egress_port_0();
            @defaultonly NoAction();
        }
        key = {
            meta.standard_metadata.ingress_port: exact @name("meta.standard_metadata.ingress_port") ;
        }
        default_action = NoAction();
    }
    apply {
        repeater_0.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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
