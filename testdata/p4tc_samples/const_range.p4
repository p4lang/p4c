// SPDX-License-Identifier: Apache-2.0
#include <core.p4>
#include <tc/pna.p4>

#ifndef CONST_RANGE_SIZE
#define CONST_RANGE_SIZE 32
#endif

header Ethernet {
    bit<48> dst;
    bit<48> src;
    bit<16> etherType;
}

header Fields {
    bit<8> left;
    bit<8> right;
    bit<8> selector;
    bit<8> flags;
    bit<8> result;
}

struct Headers { Ethernet ethernet; Fields fields; }
struct Metadata {}

parser Parser(packet_in packet, out Headers hdr, inout Metadata meta,
              in pna_main_parser_input_metadata_t istd) {
    state start {
        packet.extract(hdr.ethernet);
        packet.extract(hdr.fields);
        verify(hdr.ethernet.etherType == 0x88b5, error.NoMatch);
        transition accept;
    }
}

control cb(inout Headers hdr, inout Metadata meta,
           in pna_main_input_metadata_t istd,
           inout pna_main_output_metadata_t ostd) {
    action hit(bit<8> value) { hdr.fields.result = value; }
    action miss() { hdr.fields.result = 0; }

#ifdef CONST_RANGE_NMASKS
    @nummask(CONST_RANGE_NMASKS)
#endif
    table ranges {
        key = {
            hdr.fields.left : range;
            hdr.fields.right : range;
            hdr.fields.selector : exact;
            hdr.fields.flags : ternary;
        }
        actions = { hit; miss; }
        default_action = miss();
        size = CONST_RANGE_SIZE;
        const entries = {
            (1, 3, 7, 0xab): hit(21);
            (1..8, 3..5, 7, 0xa0 &&& 0xf0): hit(23);
            (1 &&& 2, _, 7, _): hit(26);
            (_, _, 7, _): hit(31);
        }
    }

    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
            return;
        }
        ranges.apply();
        send_to_port(istd.input_port);
    }
}

control Deparser(packet_out packet, inout Headers hdr, in Metadata meta,
                 in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.fields);
    }
}

PNA_NIC(Parser(), cb(), Deparser()) main;
