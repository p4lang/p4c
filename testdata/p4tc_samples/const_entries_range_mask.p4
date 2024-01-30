#include <core.p4>
#include <tc/pna.p4>

header hdr {
    bit<8>  e;
    bit<16> t;
    bit<8>  l;
    bit<8> r;
    bit<8>  v;
}

struct Header_t {
    hdr h;
}
struct Meta_t {}

parser MainParserImpl(packet_in b, out Header_t h, inout Meta_t m, in pna_main_parser_input_metadata_t istd) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control MainControlImpl(inout Header_t h, inout Meta_t m,
                        in    pna_main_input_metadata_t istd,
                        inout pna_main_output_metadata_t ostd) {

    action a() { h.h.e = 0; }
    action a_with_control_params(bit<16> x) { h.h.t = x; }

    table t_range {

 	key = {
            h.h.r : range;
        }

        actions = {
            a;
            a_with_control_params;
        }

        default_action = a;

        const entries = {
            1 : a_with_control_params(21);
            6: a_with_control_params(22);
            15   : a_with_control_params(24);
            1..8    : a_with_control_params(23);
            1 &&& 2    : a_with_control_params(26);
        }
    }

    apply {
        t_range.apply();
    }
}

/*********************  D E P A R S E R  ************************/

control MainDeparserImpl(
    packet_out b, inout Header_t h, in Meta_t m, in pna_main_output_metadata_t ostd)
{
    apply {}
}

/************ F I N A L   P A C K A G E ******************************/


// BEGIN:Package_Instantiation_Example
PNA_NIC(
    MainParserImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;
// END:Package_Instantiation_Example
