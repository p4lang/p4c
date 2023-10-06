#include <core.p4>

struct standard_metadata_t {
    bit<8> foo;
}

parser Parser<H, M>(packet_in b, out H parsedHdr, inout M meta, inout standard_metadata_t standard_metadata);
control VerifyChecksum<H, M>(inout H hdr, inout M meta);
control EmptyVerifyChecksum<H, M>(inout H hdr, inout M meta) {
    apply {
    }
}

control Ingress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Egress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control ComputeChecksum<H, M>(inout H hdr, inout M meta);
control Deparser<H>(packet_out b, in H hdr);
package MySwitch<H, M>(Parser<H, M> p, VerifyChecksum<H, M> vr=EmptyVerifyChecksum<H, M>(), Ingress<H, M> ig, Egress<H, M> eg, ComputeChecksum<H, M> ck, Deparser<H> dep);
header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    @hidden action issue19142l109() {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_issue19142l109 {
        actions = {
            issue19142l109();
        }
        const default_action = issue19142l109();
    }
    apply {
        tbl_issue19142l109.apply();
    }
}

control EmptyVerifyChecksum_0(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

MySwitch<headers_t, metadata_t>(p = parserImpl(), vr = EmptyVerifyChecksum_0(), ig = ingressImpl(), eg = egressImpl(), ck = updateChecksum(), dep = deparserImpl()) main;
