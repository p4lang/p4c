#include <core.p4>

// Copied from 2017-Jul-17 version of the P4_16 spec .mdk file, in the
// section "Very Simple Switch Architecture"

typedef bit<4> PortId;

struct InControl {
    PortId inputPort;
}

struct OutControl {
    PortId outputPort;
}

// Copied from 2017-Jul-17 version of the P4_16 spec .mdk file, in the
// section "Example architecture description"

parser Parser<IH>(packet_in b, out IH parsedHeaders);
// ingress match-action pipeline
control IPipe<T, IH, OH>(in IH inputHeaders,
                         in InControl inCtrl,
                         out OH outputHeaders,
                         out T toEgress,
                         out OutControl outCtrl);
// egress match-action pipeline
control EPipe<T, IH, OH>(in IH inputHeaders,
                         in InControl inCtrl,
                         in T fromIngress,
                         out OH outputHeaders,
                         out OutControl outCtrl);
control Deparser<OH>(in OH outputHeaders, packet_out b);
package Ingress<T, IH, OH>(Parser<IH> p,
                           IPipe<T, IH, OH> map,
                           Deparser<OH> d);
package Egress<T, IH, OH>(Parser<IH> p,
                          EPipe<T, IH, OH> map,
                          Deparser<OH> d);
package Switch<T>(Ingress<T, _, _> ingress, Egress<T, _, _> egress);

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

// Make all of the types ing_in_headers, ing_out_headers,
// egr_in_headers, and egr_out_headers at least slightly different
// from each other, so they cannot be unified.  This helps us to know
// whether we have type parameters for constructors correct in later
// code.

struct ing_in_headers {
    ethernet_t       ethernet;
    bit<10> a0;
}

struct ing_out_headers {
    ethernet_t       ethernet;
    bit<11> a1;
}

struct egr_in_headers {
    ethernet_t       ethernet;
    bit<12> a2;
}

struct egr_out_headers {
    ethernet_t       ethernet;
    bit<13> a3;
}

struct ing_to_egr {
    PortId x;
}

parser ing_parse(packet_in buffer,
                 out ing_in_headers parsed_hdr)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        transition accept;
    }
}

control ingress(in ing_in_headers ihdr,
                in InControl inCtrl,
                out ing_out_headers ohdr,
                out ing_to_egr toEgress,
                out OutControl outCtrl)
{
    apply {
        ohdr.ethernet = ihdr.ethernet;
        toEgress.x = inCtrl.inputPort;
        outCtrl.outputPort = inCtrl.inputPort;
    }
}

control ing_deparse(in ing_out_headers ohdr,
                    packet_out b)
{
    apply { b.emit(ohdr.ethernet); }
}

parser egr_parse(packet_in buffer,
                 out egr_in_headers parsed_hdr)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        transition accept;
    }
}

control egress(in egr_in_headers ihdr,
               in InControl inCtrl,
               in ing_to_egr fromIngress,
               out egr_out_headers ohdr,
               out OutControl outCtrl)
{
    apply {
        ohdr.ethernet = ihdr.ethernet;
        outCtrl.outputPort = fromIngress.x;
    }
}

control egr_deparse(in egr_out_headers ohdr,
                    packet_out b)
{
    apply { b.emit(ohdr.ethernet); }
}

// It is normal for the compiler to give "unused instance" warnings
// messages for the package instantiations for Ingress and Egress
// below, if the instances are not used later.

Ingress(ing_parse(), ingress(), ing_deparse()) ig1;
Ingress<ing_to_egr, ing_in_headers, ing_out_headers>
    (ing_parse(), ingress(), ing_deparse()) ig2;

Egress(egr_parse(), egress(), egr_deparse()) eg1;
Egress<ing_to_egr, egr_in_headers, egr_out_headers>
    (egr_parse(), egress(), egr_deparse()) eg2;

// Next instantiation gives error, as expected:
// "Cannot unify struct egr_in_headers to struct ing_in_headers"

//Egress<ing_to_egr, ing_in_headers, egr_out_headers>
//    (egr_parse(), egress(), egr_deparse()) eg3;

// If you try any one of the attempted instantiations of package
// Switch below, it causes the latest version of p4test as of
// 2017-Jul-19 to give an error message like this;

// example-arch-use.p4(140): error: main: Cannot unify package Egress to package Egress
// Switch(ig1, eg2) main;
//                  ^^^^

Switch(ig1, eg1) main;
//Switch(ig1, eg2) main;
//Switch(ig2, eg1) main;
//Switch(ig2, eg2) main;
//Switch<ing_to_egr>(ig1, eg1) main;
//Switch(Ingress(ing_parse(), ingress(), ing_deparse()),
//       Egress(egr_parse(), egress(), egr_deparse())) main;
