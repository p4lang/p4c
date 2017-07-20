#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> f3;
    bit<16> f4;
    bit<8>  f5;
    bit<8>  f6;
    bit<4>  f7;
    bit<4>  f8;
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ethertype;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("data") 
    data_t     data;
    @name("ethernet") 
    ethernet_t ethernet;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".data") state data {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition data;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".route_eth") action route_eth_0(bit<9> egress_spec, bit<48> src_addr) {
        meta.standard_metadata.egress_spec = egress_spec;
        hdr.ethernet.src_addr = src_addr;
    }
    @name(".noop") action noop_0() {
    }
    @name(".setf2") action setf2_0(bit<32> val) {
        hdr.data.f2 = val;
    }
    @name(".setf1") action setf1_0(bit<32> val) {
        hdr.data.f1 = val;
    }
    @name(".routing") table routing_0 {
        actions = {
            route_eth_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.ethernet.dst_addr: lpm @name("hdr.ethernet.dst_addr") ;
        }
        default_action = NoAction();
    }
    @name(".test1") table test1_0 {
        actions = {
            setf2_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: exact @name("hdr.data.f1") ;
        }
        default_action = NoAction();
    }
    @name(".test2") table test2_0 {
        actions = {
            setf1_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2: exact @name("hdr.data.f2") ;
        }
        default_action = NoAction();
    }
    apply {
        routing_0.apply();
        if (hdr.data.f5 != hdr.data.f6) 
            test1_0.apply();
        else 
            test2_0.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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
