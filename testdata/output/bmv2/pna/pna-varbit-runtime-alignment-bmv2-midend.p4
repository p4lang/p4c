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
    @hidden action pnavarbitruntimealignmentbmv2l41() {
        send_to_port(32w0);
    }
    @hidden action pnavarbitruntimealignmentbmv2l44() {
        send_to_port(32w1);
    }
    @hidden action pnavarbitruntimealignmentbmv2l47() {
        send_to_port(32w2);
    }
    @hidden action pnavarbitruntimealignmentbmv2l50() {
        send_to_port(32w3);
    }
    @hidden action pnavarbitruntimealignmentbmv2l52() {
        send_to_port(32w4);
    }
    @hidden table tbl_pnavarbitruntimealignmentbmv2l41 {
        actions = {
            pnavarbitruntimealignmentbmv2l41();
        }
        const default_action = pnavarbitruntimealignmentbmv2l41();
    }
    @hidden table tbl_pnavarbitruntimealignmentbmv2l44 {
        actions = {
            pnavarbitruntimealignmentbmv2l44();
        }
        const default_action = pnavarbitruntimealignmentbmv2l44();
    }
    @hidden table tbl_pnavarbitruntimealignmentbmv2l47 {
        actions = {
            pnavarbitruntimealignmentbmv2l47();
        }
        const default_action = pnavarbitruntimealignmentbmv2l47();
    }
    @hidden table tbl_pnavarbitruntimealignmentbmv2l50 {
        actions = {
            pnavarbitruntimealignmentbmv2l50();
        }
        const default_action = pnavarbitruntimealignmentbmv2l50();
    }
    @hidden table tbl_pnavarbitruntimealignmentbmv2l52 {
        actions = {
            pnavarbitruntimealignmentbmv2l52();
        }
        const default_action = pnavarbitruntimealignmentbmv2l52();
    }
    apply {
        if (istd.parser_error == error.NoError && hdr.variable.isValid()) {
            tbl_pnavarbitruntimealignmentbmv2l41.apply();
        } else if (istd.parser_error == error.ParserInvalidArgument && !hdr.variable.isValid()) {
            tbl_pnavarbitruntimealignmentbmv2l44.apply();
        } else if (istd.parser_error == error.PacketTooShort && !hdr.variable.isValid()) {
            tbl_pnavarbitruntimealignmentbmv2l47.apply();
        } else if (istd.parser_error == error.HeaderTooShort && !hdr.variable.isValid()) {
            tbl_pnavarbitruntimealignmentbmv2l50.apply();
        } else {
            tbl_pnavarbitruntimealignmentbmv2l52.apply();
        }
    }
}

control MainDeparserImpl(packet_out packet, inout headers_t hdr, in empty_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnavarbitruntimealignmentbmv2l62() {
        packet.emit<length_t>(hdr.length);
        packet.emit<variable_t>(hdr.variable);
    }
    @hidden table tbl_pnavarbitruntimealignmentbmv2l62 {
        actions = {
            pnavarbitruntimealignmentbmv2l62();
        }
        const default_action = pnavarbitruntimealignmentbmv2l62();
    }
    apply {
        tbl_pnavarbitruntimealignmentbmv2l62.apply();
    }
}

PNA_NIC<headers_t, empty_t>(MainParserImpl(), MainControlImpl(), MainDeparserImpl()) main;
