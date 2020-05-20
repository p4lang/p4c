#include <core.p4>
#include <v1model.p4>


/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

header H {
    bit<8>  h;
}

struct metadata {
}

struct headers {
    H h;
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/



parser PROTParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        transition state_1;
    }
    state state_1 {
        transition state_2;
    }
    state state_2 {
        transition state_3;
    }
    state state_3 {
        transition state_2;
    }

}


/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control PROTVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}


/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control PROTIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    apply {
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control PROTEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
   apply {
    }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control PROTComputeChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}


/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control PROTDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
PROTParser(),
PROTVerifyChecksum(),
PROTIngress(),
PROTEgress(),
PROTComputeChecksum(),
PROTDeparser()
) main;