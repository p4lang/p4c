#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> mac_addr_t;
header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16>    etherType;
}

header ml_hdr_t {
    bit<8> idx1;
    bit<8> idx2;
}

header vec_e_t {
    bit<8> e;
}

struct headers {
    ethernet_t ethernet;
    ml_hdr_t   ml;
    vec_e_t[8] vector;
}

struct metadata_t {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<ml_hdr_t>(hdr.ml);
        packet.extract<vec_e_t>(hdr.vector[0]);
        packet.extract<vec_e_t>(hdr.vector[1]);
        packet.extract<vec_e_t>(hdr.vector[2]);
        packet.extract<vec_e_t>(hdr.vector[3]);
        packet.extract<vec_e_t>(hdr.vector[4]);
        packet.extract<vec_e_t>(hdr.vector[5]);
        packet.extract<vec_e_t>(hdr.vector[6]);
        packet.extract<vec_e_t>(hdr.vector[7]);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @hidden action runtimeindex2bmv2l69() {
        if (hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1) == 8w0) {
            hdr.vector[0].e = hdr.ethernet.etherType[15:8] + 8w7;
        } else if (hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1) == 8w1) {
            hdr.vector[1].e = hdr.ethernet.etherType[15:8] + 8w7;
        } else if (hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1) == 8w2) {
            hdr.vector[2].e = hdr.ethernet.etherType[15:8] + 8w7;
        } else if (hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1) == 8w3) {
            hdr.vector[3].e = hdr.ethernet.etherType[15:8] + 8w7;
        } else if (hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1) == 8w4) {
            hdr.vector[4].e = hdr.ethernet.etherType[15:8] + 8w7;
        } else if (hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1) == 8w5) {
            hdr.vector[5].e = hdr.ethernet.etherType[15:8] + 8w7;
        } else if (hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1) == 8w6) {
            hdr.vector[6].e = hdr.ethernet.etherType[15:8] + 8w7;
        } else if (hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1) == 8w7) {
            hdr.vector[7].e = hdr.ethernet.etherType[15:8] + 8w7;
        }
        if ((hdr.ml.idx2 ^ 8w0x7) & 8w0x7 == 8w0) {
            hdr.ethernet.etherType[7:0] = hdr.vector[0].e;
        } else if ((hdr.ml.idx2 ^ 8w0x7) & 8w0x7 == 8w1) {
            hdr.ethernet.etherType[7:0] = hdr.vector[1].e;
        } else if ((hdr.ml.idx2 ^ 8w0x7) & 8w0x7 == 8w2) {
            hdr.ethernet.etherType[7:0] = hdr.vector[2].e;
        } else if ((hdr.ml.idx2 ^ 8w0x7) & 8w0x7 == 8w3) {
            hdr.ethernet.etherType[7:0] = hdr.vector[3].e;
        } else if ((hdr.ml.idx2 ^ 8w0x7) & 8w0x7 == 8w4) {
            hdr.ethernet.etherType[7:0] = hdr.vector[4].e;
        } else if ((hdr.ml.idx2 ^ 8w0x7) & 8w0x7 == 8w5) {
            hdr.ethernet.etherType[7:0] = hdr.vector[5].e;
        } else if ((hdr.ml.idx2 ^ 8w0x7) & 8w0x7 == 8w6) {
            hdr.ethernet.etherType[7:0] = hdr.vector[6].e;
        } else if ((hdr.ml.idx2 ^ 8w0x7) & 8w0x7 == 8w7) {
            hdr.ethernet.etherType[7:0] = hdr.vector[7].e;
        }
        if (hdr.ethernet.dstAddr[39:32] & 8w0x7 == 8w0) {
            if (hdr.vector[0].e & 8w0x7 == 8w0) {
                hdr.vector[0].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[0].e & 8w0x7 == 8w1) {
                hdr.vector[1].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[0].e & 8w0x7 == 8w2) {
                hdr.vector[2].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[0].e & 8w0x7 == 8w3) {
                hdr.vector[3].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[0].e & 8w0x7 == 8w4) {
                hdr.vector[4].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[0].e & 8w0x7 == 8w5) {
                hdr.vector[5].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[0].e & 8w0x7 == 8w6) {
                hdr.vector[6].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[0].e & 8w0x7 == 8w7) {
                hdr.vector[7].e = hdr.ethernet.dstAddr[47:40];
            }
        } else if (hdr.ethernet.dstAddr[39:32] & 8w0x7 == 8w1) {
            if (hdr.vector[1].e & 8w0x7 == 8w0) {
                hdr.vector[0].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[1].e & 8w0x7 == 8w1) {
                hdr.vector[1].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[1].e & 8w0x7 == 8w2) {
                hdr.vector[2].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[1].e & 8w0x7 == 8w3) {
                hdr.vector[3].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[1].e & 8w0x7 == 8w4) {
                hdr.vector[4].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[1].e & 8w0x7 == 8w5) {
                hdr.vector[5].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[1].e & 8w0x7 == 8w6) {
                hdr.vector[6].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[1].e & 8w0x7 == 8w7) {
                hdr.vector[7].e = hdr.ethernet.dstAddr[47:40];
            }
        } else if (hdr.ethernet.dstAddr[39:32] & 8w0x7 == 8w2) {
            if (hdr.vector[2].e & 8w0x7 == 8w0) {
                hdr.vector[0].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[2].e & 8w0x7 == 8w1) {
                hdr.vector[1].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[2].e & 8w0x7 == 8w2) {
                hdr.vector[2].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[2].e & 8w0x7 == 8w3) {
                hdr.vector[3].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[2].e & 8w0x7 == 8w4) {
                hdr.vector[4].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[2].e & 8w0x7 == 8w5) {
                hdr.vector[5].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[2].e & 8w0x7 == 8w6) {
                hdr.vector[6].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[2].e & 8w0x7 == 8w7) {
                hdr.vector[7].e = hdr.ethernet.dstAddr[47:40];
            }
        } else if (hdr.ethernet.dstAddr[39:32] & 8w0x7 == 8w3) {
            if (hdr.vector[3].e & 8w0x7 == 8w0) {
                hdr.vector[0].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[3].e & 8w0x7 == 8w1) {
                hdr.vector[1].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[3].e & 8w0x7 == 8w2) {
                hdr.vector[2].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[3].e & 8w0x7 == 8w3) {
                hdr.vector[3].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[3].e & 8w0x7 == 8w4) {
                hdr.vector[4].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[3].e & 8w0x7 == 8w5) {
                hdr.vector[5].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[3].e & 8w0x7 == 8w6) {
                hdr.vector[6].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[3].e & 8w0x7 == 8w7) {
                hdr.vector[7].e = hdr.ethernet.dstAddr[47:40];
            }
        } else if (hdr.ethernet.dstAddr[39:32] & 8w0x7 == 8w4) {
            if (hdr.vector[4].e & 8w0x7 == 8w0) {
                hdr.vector[0].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[4].e & 8w0x7 == 8w1) {
                hdr.vector[1].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[4].e & 8w0x7 == 8w2) {
                hdr.vector[2].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[4].e & 8w0x7 == 8w3) {
                hdr.vector[3].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[4].e & 8w0x7 == 8w4) {
                hdr.vector[4].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[4].e & 8w0x7 == 8w5) {
                hdr.vector[5].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[4].e & 8w0x7 == 8w6) {
                hdr.vector[6].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[4].e & 8w0x7 == 8w7) {
                hdr.vector[7].e = hdr.ethernet.dstAddr[47:40];
            }
        } else if (hdr.ethernet.dstAddr[39:32] & 8w0x7 == 8w5) {
            if (hdr.vector[5].e & 8w0x7 == 8w0) {
                hdr.vector[0].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[5].e & 8w0x7 == 8w1) {
                hdr.vector[1].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[5].e & 8w0x7 == 8w2) {
                hdr.vector[2].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[5].e & 8w0x7 == 8w3) {
                hdr.vector[3].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[5].e & 8w0x7 == 8w4) {
                hdr.vector[4].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[5].e & 8w0x7 == 8w5) {
                hdr.vector[5].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[5].e & 8w0x7 == 8w6) {
                hdr.vector[6].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[5].e & 8w0x7 == 8w7) {
                hdr.vector[7].e = hdr.ethernet.dstAddr[47:40];
            }
        } else if (hdr.ethernet.dstAddr[39:32] & 8w0x7 == 8w6) {
            if (hdr.vector[6].e & 8w0x7 == 8w0) {
                hdr.vector[0].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[6].e & 8w0x7 == 8w1) {
                hdr.vector[1].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[6].e & 8w0x7 == 8w2) {
                hdr.vector[2].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[6].e & 8w0x7 == 8w3) {
                hdr.vector[3].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[6].e & 8w0x7 == 8w4) {
                hdr.vector[4].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[6].e & 8w0x7 == 8w5) {
                hdr.vector[5].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[6].e & 8w0x7 == 8w6) {
                hdr.vector[6].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[6].e & 8w0x7 == 8w7) {
                hdr.vector[7].e = hdr.ethernet.dstAddr[47:40];
            }
        } else if (hdr.ethernet.dstAddr[39:32] & 8w0x7 == 8w7) {
            if (hdr.vector[7].e & 8w0x7 == 8w0) {
                hdr.vector[0].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[7].e & 8w0x7 == 8w1) {
                hdr.vector[1].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[7].e & 8w0x7 == 8w2) {
                hdr.vector[2].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[7].e & 8w0x7 == 8w3) {
                hdr.vector[3].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[7].e & 8w0x7 == 8w4) {
                hdr.vector[4].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[7].e & 8w0x7 == 8w5) {
                hdr.vector[5].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[7].e & 8w0x7 == 8w6) {
                hdr.vector[6].e = hdr.ethernet.dstAddr[47:40];
            } else if (hdr.vector[7].e & 8w0x7 == 8w7) {
                hdr.vector[7].e = hdr.ethernet.dstAddr[47:40];
            }
        }
    }
    @hidden table tbl_runtimeindex2bmv2l69 {
        actions = {
            runtimeindex2bmv2l69();
        }
        const default_action = runtimeindex2bmv2l69();
    }
    apply {
        tbl_runtimeindex2bmv2l69.apply();
    }
}

control egress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ml_hdr_t>(hdr.ml);
        packet.emit<vec_e_t>(hdr.vector[0]);
        packet.emit<vec_e_t>(hdr.vector[1]);
        packet.emit<vec_e_t>(hdr.vector[2]);
        packet.emit<vec_e_t>(hdr.vector[3]);
        packet.emit<vec_e_t>(hdr.vector[4]);
        packet.emit<vec_e_t>(hdr.vector[5]);
        packet.emit<vec_e_t>(hdr.vector[6]);
        packet.emit<vec_e_t>(hdr.vector[7]);
    }
}

V1Switch<headers, metadata_t>(MyParser(), MyVerifyChecksum(), ingress(), egress(), MyComputeChecksum(), MyDeparser()) main;

