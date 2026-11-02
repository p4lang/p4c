#include <core.p4>

#include <bmv2/pna.p4>

header length_t {
    bit<8> bits;
}

header variable_t {
    bit<8>     tag;
    varbit<64> data;
}

struct headers_t {
    length_t   length;
    variable_t variable;
}

struct empty_t {
}

parser MainParserImpl(packet_in packet, out headers_t hdr, inout empty_t user_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        packet.extract<length_t>(hdr.length);
        packet.extract<variable_t>(hdr.variable, (bit<32>)hdr.length.bits);
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout empty_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error == error.NoError && hdr.variable.isValid()) {
            send_to_port((PortId_t)32w0);
        } else if (istd.parser_error == error.ParserInvalidArgument && !hdr.variable.isValid()) {
            send_to_port((PortId_t)32w1);
        } else if (istd.parser_error == error.PacketTooShort && !hdr.variable.isValid()) {
            send_to_port((PortId_t)32w2);
        } else if (istd.parser_error == error.HeaderTooShort && !hdr.variable.isValid()) {
            send_to_port((PortId_t)32w3);
        } else {
            send_to_port((PortId_t)32w4);
        }
    }
}

control MainDeparserImpl(packet_out packet, inout headers_t hdr, in empty_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit<length_t>(hdr.length);
        packet.emit<variable_t>(hdr.variable);
    }
}

PNA_NIC<headers_t, empty_t>(MainParserImpl(), MainControlImpl(), MainDeparserImpl()) main;
