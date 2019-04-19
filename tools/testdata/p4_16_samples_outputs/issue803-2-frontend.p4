#include <core.p4>

typedef bit<4> PortId;
struct InControl {
    PortId inputPort;
}

struct OutControl {
    PortId outputPort;
}

parser Parser<IH>(packet_in b, out IH parsedHeaders);
control IPipe<T, IH, OH>(in IH inputHeaders, in InControl inCtrl, out OH outputHeaders, out T toEgress, out OutControl outCtrl);
control EPipe<T, IH, OH>(in IH inputHeaders, in InControl inCtrl, in T fromIngress, out OH outputHeaders, out OutControl outCtrl);
control Deparser<OH>(in OH outputHeaders, packet_out b);
package Ingress<T, IH, OH>(Parser<IH> p, IPipe<T, IH, OH> map, Deparser<OH> d);
package Egress<T, IH, OH>(Parser<IH> p, EPipe<T, IH, OH> map, Deparser<OH> d);
package Switch<T>(Ingress<T, _, _> ingress, Egress<T, _, _> egress);
typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct ing_in_headers {
    ethernet_t ethernet;
    bit<10>    a0;
}

struct ing_out_headers {
    ethernet_t ethernet;
    bit<11>    a1;
}

struct egr_in_headers {
    ethernet_t ethernet;
    bit<12>    a2;
}

struct egr_out_headers {
    ethernet_t ethernet;
    bit<13>    a3;
}

struct ing_to_egr {
    PortId x;
}

parser ing_parse(packet_in buffer, out ing_in_headers parsed_hdr) {
    state start {
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition accept;
    }
}

control ingress(in ing_in_headers ihdr, in InControl inCtrl, out ing_out_headers ohdr, out ing_to_egr toEgress, out OutControl outCtrl) {
    apply {
        ohdr.ethernet = ihdr.ethernet;
        toEgress.x = inCtrl.inputPort;
        outCtrl.outputPort = inCtrl.inputPort;
    }
}

control ing_deparse(in ing_out_headers ohdr, packet_out b) {
    apply {
        b.emit<ethernet_t>(ohdr.ethernet);
    }
}

parser egr_parse(packet_in buffer, out egr_in_headers parsed_hdr) {
    state start {
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition accept;
    }
}

control egress(in egr_in_headers ihdr, in InControl inCtrl, in ing_to_egr fromIngress, out egr_out_headers ohdr, out OutControl outCtrl) {
    apply {
        ohdr.ethernet = ihdr.ethernet;
        outCtrl.outputPort = fromIngress.x;
    }
}

control egr_deparse(in egr_out_headers ohdr, packet_out b) {
    apply {
        b.emit<ethernet_t>(ohdr.ethernet);
    }
}

Ingress<ing_to_egr, ing_in_headers, ing_out_headers>(ing_parse(), ingress(), ing_deparse()) ig1;

Egress<ing_to_egr, egr_in_headers, egr_out_headers>(egr_parse(), egress(), egr_deparse()) eg1;

Switch<ing_to_egr>(ig1, eg1) main;

