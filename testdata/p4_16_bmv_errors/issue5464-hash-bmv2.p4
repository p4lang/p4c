/*
 * SPDX-FileCopyrightText: 2026 Abhishek Agarwal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Like issue5464-bmv2.p4, but the enum reaches hash() instead of verify_checksum().

extern packet_in { }
extern packet_out { }
struct standard_metadata_t { }
enum HashAlgorithm { csum16 }
extern void hash<O, T, D, M>(out O result, in HashAlgorithm algo, in T base, in D data, in M max);
parser Parser<H, M>(packet_in b, out H parsedHdr, inout M meta,
                    inout standard_metadata_t standard_metadata);
control VerifyChecksum<H, M>(inout H hdr, inout M meta);
control Ingress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Egress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control ComputeChecksum<H, M>(inout H hdr, inout M meta);
control Deparser<H>(packet_out b, in H hdr);
package V1Switch<H, M>(Parser<H, M> p, VerifyChecksum<H, M> vr, Ingress<H, M> ig, Egress<H, M> eg,
                       ComputeChecksum<H, M> ck, Deparser<H> dep);

struct headers_t { }
struct metadata_t {
    bit<16> result;
    bit<16> b;
}

parser EmptyParser(packet_in b, out headers_t hdr, inout metadata_t meta,
                   inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control EmptyVerifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control EmptyIngress(inout headers_t hdr, inout metadata_t meta,
                     inout standard_metadata_t standard_metadata) {
    apply {
        hash(meta.result, HashAlgorithm.csum16, 16w0, { meta.b }, 16w1);
    }
}

control EmptyEgress(inout headers_t hdr, inout metadata_t meta,
                    inout standard_metadata_t standard_metadata) {
    apply { }
}

control EmptyComputeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control EmptyDeparser(packet_out b, in headers_t hdr) {
    apply { }
}

V1Switch(EmptyParser(), EmptyVerifyChecksum(), EmptyIngress(), EmptyEgress(),
         EmptyComputeChecksum(), EmptyDeparser()) main;
