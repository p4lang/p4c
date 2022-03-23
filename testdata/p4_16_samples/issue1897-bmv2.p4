/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

header addr_type_t {
    bit<8> dstType;
    bit<8> srcType;
}

header addr_ipv4_t {
    bit<32> addr;
}

header addr_ipv6_t {
    bit<128> addr;
}

header_union addr_t {
    addr_ipv4_t ipv4;
    addr_ipv6_t ipv6;
}

struct metadata { }

struct headers {
    addr_type_t addr_type;
    addr_t      addr_dst;
    addr_t      addr_src;
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/
parser ProtAddrParser(packet_in packet,
                       in bit<8> addrType,
                       out addr_t addr) {
    state start {
        transition select(addrType) {
            0x01: ipv4;
            0x02: ipv6;
        }
    }

    state ipv4 {
        packet.extract(addr.ipv4);
        transition accept;
    }

    state ipv6 {
        packet.extract(addr.ipv6);
        transition accept;
    }
}

parser ProtParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    ProtAddrParser() addrParser;

    state start {
        packet.extract(hdr.addr_type);

        addrParser.apply(packet, hdr.addr_type.dstType, hdr.addr_dst);
        addrParser.apply(packet, hdr.addr_type.srcType, hdr.addr_src);
        transition accept;
    }
}


/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control ProtVerifyChecksum(inout headers hdr, inout metadata meta) {   
    apply {  }
}


/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control ProtIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    apply { }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control ProtEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply { }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control ProtComputeChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}


/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control ProtDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>(hdr);
    }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
ProtParser(),
ProtVerifyChecksum(),
ProtIngress(),
ProtEgress(),
ProtComputeChecksum(),
ProtDeparser()
) main;
