#include <core.p4>
#include <v1model.p4>

typedef bit<48>  EthernetAddress;

header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct Parsed_packet {
    Ethernet_h    ethernet;
}

struct mystruct1 {
    bit<4>  a;
    bit<4>  b;
}

control DeparserI(packet_out packet,
                  in Parsed_packet hdr) {
    apply { packet.emit(hdr.ethernet); }
}

parser parserI(packet_in pkt,
               out Parsed_packet hdr,
               inout mystruct1 meta,
               inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr,
                 inout mystruct1 meta,
                 inout standard_metadata_t stdmeta) {
    action foo (bit<16> bar) {
        if (bar == 16w0xf00d) {
            hdr.ethernet.srcAddr = 48w0xdeadbeeff00d;
            // tested with 2017-Mar-30 p4test and p4c-bm2-ss

            // With following line, there is crash.

            // Commented out, there is no crash.  There is a warning
            // from p4c-bm2-ss that it is removing unused action
            // parameter 'bar' for compatibility reasons, but that
            // seems wrong, given that the value of bar must be used
            // to implement the action's behavior correctly.
           return;
        }
        hdr.ethernet.srcAddr = ~48w0xdeadbeeff00d;
    }
    table tbl1 {
        key = { }
        actions = { foo; NoAction; }
        default_action = NoAction;
    }
    apply {
        tbl1.apply();
        return;
    }
}

control cEgress(inout Parsed_packet hdr,
                inout mystruct1 meta,
                inout standard_metadata_t stdmeta) {
    apply { }
}

control vc(inout Parsed_packet hdr,
           inout mystruct1 meta) {
    apply { }
}

control uc(inout Parsed_packet hdr,
           inout mystruct1 meta) {
    apply { }
}

V1Switch<Parsed_packet, mystruct1>(parserI(),
                                   vc(),
                                   cIngress(),
                                   cEgress(),
                                   uc(),
                                   DeparserI()) main;
