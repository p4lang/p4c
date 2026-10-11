/*
 * SPDX-FileCopyrightText: 2026 Abhishek Agarwal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// From issue #5464: the program declares its own HashAlgorithm, no v1model.p4.

extern packet_in { }
extern packet_out { }
struct standard_metadata_t { }
enum HashAlgorithm { csum16 }
extern void verify_checksum<T, O>(in bool condition, T data, O checksum, HashAlgorithm algo);
parser Parser<H, M>(packet_in b, H parsedHdr, M meta, standard_metadata_t standard_metadata);
control VerifyChecksum<H, M>(inout H hdr, M meta);
control Ingress<H, M>(inout H hdr, M meta, standard_metadata_t standard_metadata);
control Egress<H, M>(inout H hdr, M meta, standard_metadata_t standard_metadata);
control ComputeChecksum<H, M>(inout H hdr, M meta);
control Deparser<H>(packet_out b, H hdr);
package V1Switch<H, M>(Parser<H, M> p, VerifyChecksum<H, M> vr, Ingress<H, M> ig, Egress<H, M> eg,
                       ComputeChecksum<H, M> ck, Deparser<H> dep);

struct headers_t { }
struct metadata_t {
    bit<1> result;
    bit<1> b;
}

parser EmptyParser(packet_in b, headers_t headers, metadata_t meta,
                   standard_metadata_t standard_metadata)() {
    state start { }
}

control EmptyVerifyChecksum(inout headers_t hdr, metadata_t meta)() {
    apply {
        verify_checksum(false, { }, meta.result, HashAlgorithm.csum16);
    }
}

control EmptyIngress(inout headers_t headers, metadata_t meta,
                     standard_metadata_t standard_metadata)() {
    apply { }
}

control EmptyEgress(inout headers_t hdr, metadata_t meta, standard_metadata_t standard_metadata)() {
    apply { }
}

control EmptyComputeChecksum(inout headers_t hdr, metadata_t meta)() {
    apply { }
}

control EmptyDeparser(packet_out b, headers_t hdr)() {
    apply { }
}

V1Switch(EmptyParser(), EmptyVerifyChecksum(), EmptyIngress(), EmptyEgress(),
         EmptyComputeChecksum(), EmptyDeparser()) main;
