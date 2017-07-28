#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
}

struct __metadataImpl {
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".cnt") direct_counter(CounterType.bytes) cnt;
    @name(".act") action act(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".act") action act_0(bit<9> port) {
        cnt.count();
        standard_metadata.egress_spec = port;
    }
    @name(".tab1") table tab1 {
        actions = {
            act_0;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 128;
        @name(".cnt") counters = direct_counter(CounterType.bytes);
    }
    apply {
        tab1.apply();
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit(hdr.ethernet);
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

V1Switch(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
