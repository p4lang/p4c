#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header custom_var_len_t {
    varbit<16> options;
}

struct headers_t {
    ethernet_t       ethernet;
    custom_var_len_t custom_var_len;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract(hdr.ethernet);
        transition parse_custom_variable_len_header;
    }
    state parse_custom_variable_len_header {
        packet.extract(hdr.custom_var_len, (bit<32>)hdr.ethernet.etherType[7:0]);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    bit<8> error_as_int;
    apply {
        stdmeta.egress_spec = 1;
        if (stdmeta.parser_error == error.NoError) {
            error_as_int = 0;
        } else if (stdmeta.parser_error == error.PacketTooShort) {
            error_as_int = 1;
        } else if (stdmeta.parser_error == error.NoMatch) {
            error_as_int = 2;
        } else if (stdmeta.parser_error == error.StackOutOfBounds) {
            error_as_int = 3;
        } else if (stdmeta.parser_error == error.HeaderTooShort) {
            error_as_int = 4;
        } else if (stdmeta.parser_error == error.ParserTimeout) {
            error_as_int = 5;
        } else if (stdmeta.parser_error == error.ParserInvalidArgument) {
            error_as_int = 6;
        } else {
            error_as_int = 7;
        }
        hdr.ethernet.dstAddr[7:0] = error_as_int;
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
    apply {
        packet.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;

