#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct my_metadata_t {
    bit<8>  fwdHopCount;
    bit<8>  revHopCount;
    bit<16> headerLen;
}

header axon_head_t {
    bit<64> preamble;
    bit<8>  axonType;
    bit<16> axonLength;
    bit<8>  fwdHopCount;
    bit<8>  revHopCount;
}

header axon_hop_t {
    bit<8> port;
}

struct metadata {
    bit<8>  _my_metadata_fwdHopCount0;
    bit<8>  _my_metadata_revHopCount1;
    bit<16> _my_metadata_headerLen2;
}

struct headers {
    @name(".axon_head") 
    axon_head_t    axon_head;
    @name(".axon_fwdHop") 
    axon_hop_t[64] axon_fwdHop;
    @name(".axon_revHop") 
    axon_hop_t[64] axon_revHop;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ParserImpl.tmp_0") bit<64> tmp_0;
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    state parse_fwdHop {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w0]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop1;
    }
    state parse_fwdHop1 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w1]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop2;
    }
    state parse_fwdHop2 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w2]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop3;
    }
    state parse_fwdHop3 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w3]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop4;
    }
    state parse_fwdHop4 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w4]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop5;
    }
    state parse_fwdHop5 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w5]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop6;
    }
    state parse_fwdHop6 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w6]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop7;
    }
    state parse_fwdHop7 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w7]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop8;
    }
    state parse_fwdHop8 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w8]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop9;
    }
    state parse_fwdHop9 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w9]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop10;
    }
    state parse_fwdHop10 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w10]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop11;
    }
    state parse_fwdHop11 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w11]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop12;
    }
    state parse_fwdHop12 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w12]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop13;
    }
    state parse_fwdHop13 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w13]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop14;
    }
    state parse_fwdHop14 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w14]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop15;
    }
    state parse_fwdHop15 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w15]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop16;
    }
    state parse_fwdHop16 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w16]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop17;
    }
    state parse_fwdHop17 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w17]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop18;
    }
    state parse_fwdHop18 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w18]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop19;
    }
    state parse_fwdHop19 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w19]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop20;
    }
    state parse_fwdHop20 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w20]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop21;
    }
    state parse_fwdHop21 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w21]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop22;
    }
    state parse_fwdHop22 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w22]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop23;
    }
    state parse_fwdHop23 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w23]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop24;
    }
    state parse_fwdHop24 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w24]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop25;
    }
    state parse_fwdHop25 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w25]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop26;
    }
    state parse_fwdHop26 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w26]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop27;
    }
    state parse_fwdHop27 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w27]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop28;
    }
    state parse_fwdHop28 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w28]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop29;
    }
    state parse_fwdHop29 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w29]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop30;
    }
    state parse_fwdHop30 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w30]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop31;
    }
    state parse_fwdHop31 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w31]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop32;
    }
    state parse_fwdHop32 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w32]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop33;
    }
    state parse_fwdHop33 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w33]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop34;
    }
    state parse_fwdHop34 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w34]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop35;
    }
    state parse_fwdHop35 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w35]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop36;
    }
    state parse_fwdHop36 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w36]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop37;
    }
    state parse_fwdHop37 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w37]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop38;
    }
    state parse_fwdHop38 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w38]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop39;
    }
    state parse_fwdHop39 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w39]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop40;
    }
    state parse_fwdHop40 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w40]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop41;
    }
    state parse_fwdHop41 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w41]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop42;
    }
    state parse_fwdHop42 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w42]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop43;
    }
    state parse_fwdHop43 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w43]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop44;
    }
    state parse_fwdHop44 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w44]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop45;
    }
    state parse_fwdHop45 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w45]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop46;
    }
    state parse_fwdHop46 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w46]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop47;
    }
    state parse_fwdHop47 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w47]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop48;
    }
    state parse_fwdHop48 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w48]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop49;
    }
    state parse_fwdHop49 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w49]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop50;
    }
    state parse_fwdHop50 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w50]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop51;
    }
    state parse_fwdHop51 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w51]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop52;
    }
    state parse_fwdHop52 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w52]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop53;
    }
    state parse_fwdHop53 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w53]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop54;
    }
    state parse_fwdHop54 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w54]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop55;
    }
    state parse_fwdHop55 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w55]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop56;
    }
    state parse_fwdHop56 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w56]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop57;
    }
    state parse_fwdHop57 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w57]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop58;
    }
    state parse_fwdHop58 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w58]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop59;
    }
    state parse_fwdHop59 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w59]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop60;
    }
    state parse_fwdHop60 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w60]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop61;
    }
    state parse_fwdHop61 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w61]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop62;
    }
    state parse_fwdHop62 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w62]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop63;
    }
    state parse_fwdHop63 {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop[32w63]);
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop64;
    }
    state parse_fwdHop64 {
        transition stateOutOfBound;
    }
    state parse_head {
        packet.extract<axon_head_t>(hdr.axon_head);
        meta._my_metadata_fwdHopCount0 = hdr.axon_head.fwdHopCount;
        meta._my_metadata_revHopCount1 = hdr.axon_head.revHopCount;
        meta._my_metadata_headerLen2 = (bit<16>)(8w2 + hdr.axon_head.fwdHopCount + hdr.axon_head.revHopCount);
        transition select(hdr.axon_head.fwdHopCount) {
            8w0: accept;
            default: parse_next_fwdHop;
        }
    }
    state parse_next_fwdHop {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop;
        }
    }
    state parse_next_fwdHop1 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop1;
        }
    }
    state parse_next_fwdHop2 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop2;
        }
    }
    state parse_next_fwdHop3 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop3;
        }
    }
    state parse_next_fwdHop4 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop4;
        }
    }
    state parse_next_fwdHop5 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop5;
        }
    }
    state parse_next_fwdHop6 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop6;
        }
    }
    state parse_next_fwdHop7 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop7;
        }
    }
    state parse_next_fwdHop8 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop8;
        }
    }
    state parse_next_fwdHop9 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop9;
        }
    }
    state parse_next_fwdHop10 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop10;
        }
    }
    state parse_next_fwdHop11 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop11;
        }
    }
    state parse_next_fwdHop12 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop12;
        }
    }
    state parse_next_fwdHop13 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop13;
        }
    }
    state parse_next_fwdHop14 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop14;
        }
    }
    state parse_next_fwdHop15 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop15;
        }
    }
    state parse_next_fwdHop16 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop16;
        }
    }
    state parse_next_fwdHop17 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop17;
        }
    }
    state parse_next_fwdHop18 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop18;
        }
    }
    state parse_next_fwdHop19 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop19;
        }
    }
    state parse_next_fwdHop20 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop20;
        }
    }
    state parse_next_fwdHop21 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop21;
        }
    }
    state parse_next_fwdHop22 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop22;
        }
    }
    state parse_next_fwdHop23 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop23;
        }
    }
    state parse_next_fwdHop24 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop24;
        }
    }
    state parse_next_fwdHop25 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop25;
        }
    }
    state parse_next_fwdHop26 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop26;
        }
    }
    state parse_next_fwdHop27 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop27;
        }
    }
    state parse_next_fwdHop28 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop28;
        }
    }
    state parse_next_fwdHop29 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop29;
        }
    }
    state parse_next_fwdHop30 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop30;
        }
    }
    state parse_next_fwdHop31 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop31;
        }
    }
    state parse_next_fwdHop32 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop32;
        }
    }
    state parse_next_fwdHop33 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop33;
        }
    }
    state parse_next_fwdHop34 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop34;
        }
    }
    state parse_next_fwdHop35 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop35;
        }
    }
    state parse_next_fwdHop36 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop36;
        }
    }
    state parse_next_fwdHop37 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop37;
        }
    }
    state parse_next_fwdHop38 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop38;
        }
    }
    state parse_next_fwdHop39 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop39;
        }
    }
    state parse_next_fwdHop40 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop40;
        }
    }
    state parse_next_fwdHop41 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop41;
        }
    }
    state parse_next_fwdHop42 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop42;
        }
    }
    state parse_next_fwdHop43 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop43;
        }
    }
    state parse_next_fwdHop44 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop44;
        }
    }
    state parse_next_fwdHop45 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop45;
        }
    }
    state parse_next_fwdHop46 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop46;
        }
    }
    state parse_next_fwdHop47 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop47;
        }
    }
    state parse_next_fwdHop48 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop48;
        }
    }
    state parse_next_fwdHop49 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop49;
        }
    }
    state parse_next_fwdHop50 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop50;
        }
    }
    state parse_next_fwdHop51 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop51;
        }
    }
    state parse_next_fwdHop52 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop52;
        }
    }
    state parse_next_fwdHop53 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop53;
        }
    }
    state parse_next_fwdHop54 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop54;
        }
    }
    state parse_next_fwdHop55 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop55;
        }
    }
    state parse_next_fwdHop56 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop56;
        }
    }
    state parse_next_fwdHop57 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop57;
        }
    }
    state parse_next_fwdHop58 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop58;
        }
    }
    state parse_next_fwdHop59 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop59;
        }
    }
    state parse_next_fwdHop60 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop60;
        }
    }
    state parse_next_fwdHop61 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop61;
        }
    }
    state parse_next_fwdHop62 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop62;
        }
    }
    state parse_next_fwdHop63 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop63;
        }
    }
    state parse_next_fwdHop64 {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop64;
        }
    }
    state parse_next_revHop {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop;
        }
    }
    state parse_next_revHop1 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop1;
        }
    }
    state parse_next_revHop2 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop2;
        }
    }
    state parse_next_revHop3 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop3;
        }
    }
    state parse_next_revHop4 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop4;
        }
    }
    state parse_next_revHop5 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop5;
        }
    }
    state parse_next_revHop6 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop6;
        }
    }
    state parse_next_revHop7 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop7;
        }
    }
    state parse_next_revHop8 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop8;
        }
    }
    state parse_next_revHop9 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop9;
        }
    }
    state parse_next_revHop10 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop10;
        }
    }
    state parse_next_revHop11 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop11;
        }
    }
    state parse_next_revHop12 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop12;
        }
    }
    state parse_next_revHop13 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop13;
        }
    }
    state parse_next_revHop14 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop14;
        }
    }
    state parse_next_revHop15 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop15;
        }
    }
    state parse_next_revHop16 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop16;
        }
    }
    state parse_next_revHop17 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop17;
        }
    }
    state parse_next_revHop18 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop18;
        }
    }
    state parse_next_revHop19 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop19;
        }
    }
    state parse_next_revHop20 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop20;
        }
    }
    state parse_next_revHop21 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop21;
        }
    }
    state parse_next_revHop22 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop22;
        }
    }
    state parse_next_revHop23 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop23;
        }
    }
    state parse_next_revHop24 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop24;
        }
    }
    state parse_next_revHop25 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop25;
        }
    }
    state parse_next_revHop26 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop26;
        }
    }
    state parse_next_revHop27 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop27;
        }
    }
    state parse_next_revHop28 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop28;
        }
    }
    state parse_next_revHop29 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop29;
        }
    }
    state parse_next_revHop30 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop30;
        }
    }
    state parse_next_revHop31 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop31;
        }
    }
    state parse_next_revHop32 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop32;
        }
    }
    state parse_next_revHop33 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop33;
        }
    }
    state parse_next_revHop34 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop34;
        }
    }
    state parse_next_revHop35 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop35;
        }
    }
    state parse_next_revHop36 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop36;
        }
    }
    state parse_next_revHop37 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop37;
        }
    }
    state parse_next_revHop38 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop38;
        }
    }
    state parse_next_revHop39 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop39;
        }
    }
    state parse_next_revHop40 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop40;
        }
    }
    state parse_next_revHop41 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop41;
        }
    }
    state parse_next_revHop42 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop42;
        }
    }
    state parse_next_revHop43 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop43;
        }
    }
    state parse_next_revHop44 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop44;
        }
    }
    state parse_next_revHop45 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop45;
        }
    }
    state parse_next_revHop46 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop46;
        }
    }
    state parse_next_revHop47 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop47;
        }
    }
    state parse_next_revHop48 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop48;
        }
    }
    state parse_next_revHop49 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop49;
        }
    }
    state parse_next_revHop50 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop50;
        }
    }
    state parse_next_revHop51 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop51;
        }
    }
    state parse_next_revHop52 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop52;
        }
    }
    state parse_next_revHop53 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop53;
        }
    }
    state parse_next_revHop54 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop54;
        }
    }
    state parse_next_revHop55 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop55;
        }
    }
    state parse_next_revHop56 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop56;
        }
    }
    state parse_next_revHop57 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop57;
        }
    }
    state parse_next_revHop58 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop58;
        }
    }
    state parse_next_revHop59 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop59;
        }
    }
    state parse_next_revHop60 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop60;
        }
    }
    state parse_next_revHop61 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop61;
        }
    }
    state parse_next_revHop62 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop62;
        }
    }
    state parse_next_revHop63 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop63;
        }
    }
    state parse_next_revHop64 {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop64;
        }
    }
    state parse_revHop {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w0]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop1;
    }
    state parse_revHop1 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w1]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop2;
    }
    state parse_revHop2 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w2]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop3;
    }
    state parse_revHop3 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w3]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop4;
    }
    state parse_revHop4 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w4]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop5;
    }
    state parse_revHop5 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w5]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop6;
    }
    state parse_revHop6 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w6]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop7;
    }
    state parse_revHop7 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w7]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop8;
    }
    state parse_revHop8 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w8]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop9;
    }
    state parse_revHop9 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w9]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop10;
    }
    state parse_revHop10 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w10]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop11;
    }
    state parse_revHop11 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w11]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop12;
    }
    state parse_revHop12 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w12]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop13;
    }
    state parse_revHop13 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w13]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop14;
    }
    state parse_revHop14 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w14]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop15;
    }
    state parse_revHop15 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w15]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop16;
    }
    state parse_revHop16 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w16]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop17;
    }
    state parse_revHop17 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w17]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop18;
    }
    state parse_revHop18 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w18]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop19;
    }
    state parse_revHop19 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w19]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop20;
    }
    state parse_revHop20 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w20]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop21;
    }
    state parse_revHop21 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w21]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop22;
    }
    state parse_revHop22 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w22]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop23;
    }
    state parse_revHop23 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w23]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop24;
    }
    state parse_revHop24 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w24]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop25;
    }
    state parse_revHop25 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w25]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop26;
    }
    state parse_revHop26 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w26]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop27;
    }
    state parse_revHop27 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w27]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop28;
    }
    state parse_revHop28 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w28]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop29;
    }
    state parse_revHop29 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w29]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop30;
    }
    state parse_revHop30 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w30]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop31;
    }
    state parse_revHop31 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w31]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop32;
    }
    state parse_revHop32 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w32]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop33;
    }
    state parse_revHop33 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w33]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop34;
    }
    state parse_revHop34 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w34]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop35;
    }
    state parse_revHop35 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w35]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop36;
    }
    state parse_revHop36 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w36]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop37;
    }
    state parse_revHop37 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w37]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop38;
    }
    state parse_revHop38 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w38]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop39;
    }
    state parse_revHop39 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w39]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop40;
    }
    state parse_revHop40 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w40]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop41;
    }
    state parse_revHop41 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w41]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop42;
    }
    state parse_revHop42 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w42]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop43;
    }
    state parse_revHop43 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w43]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop44;
    }
    state parse_revHop44 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w44]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop45;
    }
    state parse_revHop45 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w45]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop46;
    }
    state parse_revHop46 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w46]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop47;
    }
    state parse_revHop47 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w47]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop48;
    }
    state parse_revHop48 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w48]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop49;
    }
    state parse_revHop49 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w49]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop50;
    }
    state parse_revHop50 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w50]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop51;
    }
    state parse_revHop51 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w51]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop52;
    }
    state parse_revHop52 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w52]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop53;
    }
    state parse_revHop53 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w53]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop54;
    }
    state parse_revHop54 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w54]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop55;
    }
    state parse_revHop55 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w55]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop56;
    }
    state parse_revHop56 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w56]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop57;
    }
    state parse_revHop57 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w57]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop58;
    }
    state parse_revHop58 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w58]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop59;
    }
    state parse_revHop59 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w59]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop60;
    }
    state parse_revHop60 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w60]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop61;
    }
    state parse_revHop61 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w61]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop62;
    }
    state parse_revHop62 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w62]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop63;
    }
    state parse_revHop63 {
        packet.extract<axon_hop_t>(hdr.axon_revHop[32w63]);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
        transition parse_next_revHop64;
    }
    state parse_revHop64 {
        transition stateOutOfBound;
    }
    state start {
        tmp_0 = packet.lookahead<bit<64>>();
        transition select(tmp_0) {
            64w0: parse_head;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_1() {
        mark_to_drop(standard_metadata);
    }
    @name(".route") action route() {
        standard_metadata.egress_spec = (bit<9>)hdr.axon_fwdHop[0].port;
        hdr.axon_head.fwdHopCount = hdr.axon_head.fwdHopCount + 8w255;
        hdr.axon_fwdHop.pop_front(1);
        hdr.axon_head.revHopCount = hdr.axon_head.revHopCount + 8w1;
        hdr.axon_revHop.push_front(1);
        hdr.axon_revHop[0].setValid();
        hdr.axon_revHop[0].port = (bit<8>)standard_metadata.ingress_port;
    }
    @name(".drop_pkt") table drop_pkt_0 {
        actions = {
            _drop();
            @defaultonly NoAction_1();
        }
        size = 1;
        default_action = NoAction_1();
    }
    @name(".route_pkt") table route_pkt_0 {
        actions = {
            _drop_1();
            route();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.axon_head.isValid()     : exact @name("axon_head.$valid$") ;
            hdr.axon_fwdHop[0].isValid(): exact @name("axon_fwdHop[0].$valid$") ;
        }
        size = 1;
        default_action = NoAction_2();
    }
    apply {
        if (hdr.axon_head.axonLength != meta._my_metadata_headerLen2) {
            drop_pkt_0.apply();
        } else {
            route_pkt_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<axon_head_t>(hdr.axon_head);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[0]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[1]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[2]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[3]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[4]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[5]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[6]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[7]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[8]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[9]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[10]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[11]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[12]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[13]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[14]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[15]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[16]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[17]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[18]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[19]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[20]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[21]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[22]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[23]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[24]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[25]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[26]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[27]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[28]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[29]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[30]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[31]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[32]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[33]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[34]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[35]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[36]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[37]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[38]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[39]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[40]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[41]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[42]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[43]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[44]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[45]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[46]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[47]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[48]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[49]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[50]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[51]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[52]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[53]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[54]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[55]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[56]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[57]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[58]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[59]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[60]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[61]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[62]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[63]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[0]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[1]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[2]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[3]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[4]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[5]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[6]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[7]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[8]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[9]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[10]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[11]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[12]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[13]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[14]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[15]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[16]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[17]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[18]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[19]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[20]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[21]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[22]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[23]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[24]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[25]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[26]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[27]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[28]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[29]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[30]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[31]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[32]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[33]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[34]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[35]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[36]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[37]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[38]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[39]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[40]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[41]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[42]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[43]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[44]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[45]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[46]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[47]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[48]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[49]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[50]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[51]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[52]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[53]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[54]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[55]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[56]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[57]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[58]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[59]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[60]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[61]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[62]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[63]);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

