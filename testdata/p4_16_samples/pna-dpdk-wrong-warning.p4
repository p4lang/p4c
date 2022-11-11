#include <core.p4>
#include <dpdk/pna.p4>

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t    ethernet;
}

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

header_union hu1_t {
    h1_t h1;
    h2_t h2;
}

struct metadata_t {
}

parser ParserImpl(packet_in packet,
                  out headers_t hdr, inout metadata_t m,
    in    pna_main_parser_input_metadata_t istd) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr,
                    inout metadata_t meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd) {
    h1_t h1;
    h1_t h1b;
    h2_t h2;
    hu1_t hu1;
    hu1_t hu1b;

    h1_t[2] a1;
    h1_t[2] a1b;
    h2_t[2] a2;
    hu1_t[2] au1;
    hu1_t[2] au1b;

    apply {
        hu1b.h1 = {hdr.ethernet.dstAddr[45:38], hdr.ethernet.dstAddr[44:37]};
        // hu1b is now completely initialized, but at least some
        // versions of p4c give a warning below when hu1b is used on
        // the right-hand side of an assignment.

        // Including the following commented-out line eliminates the
        // warning, so it appears that p4c is incorrectly issuing this
        // warning because the warning-generation code has been
        // written assuming that all members of the header union must
        // be initialized, in order for the header union to be
        // considered initialized.
        //hu1b.h2 = {hdr.ethernet.dstAddr[45:38], hdr.ethernet.dstAddr[44:37]};

        hu1 = hu1b;

        // Same test case as above, but for stack with header_union
        // elements.
        au1b[0].h1 = {hdr.ethernet.dstAddr[37:30], hdr.ethernet.dstAddr[36:29]};
        au1b[1].h2 = {hdr.ethernet.dstAddr[35:28], hdr.ethernet.dstAddr[34:27]};
        
        au1 = au1b;
        
        a1 = a1b;
    }
}

control DeparserImpl(packet_out packet, 
                     in headers_t hdr,in metadata_t meta,
    in    pna_main_output_metadata_t ostd) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control PreControlImpl(
    in    headers_t  hdr,
    inout metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
    }
}

PNA_NIC(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;

