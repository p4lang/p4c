// SPDX-FileCopyrightText: 2026 sanket-jadhav-cse
// SPDX-License-Identifier: Apache-2.0

#include <core.p4>
#include <v1model.p4>

header dispatch_t { bit<8> kind; }
struct headers_t { dispatch_t dispatch; }
struct metadata_t { bit<9> port; }

// A reusable parser selects a protocol family at instantiation time.
parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta,
                  inout standard_metadata_t sm)(bit<16> profile) {
    bool enabled;
    state start {
        packet.extract(hdr.dispatch);
        transition select(profile, hdr.dispatch.kind) {
            (2, 0x10): disabled;
            (1, 0x10 &&& 0xf0): masked;
            (1, 0x10): shadowed;
            (1, 0x20 .. 0x2f): ranged;
            (1, 0xfe): late_wildcard;
            (1, 0xff): nested_dispatch;
            default: other;
        }
    }
    state disabled { meta.port = 2; transition accept; }
    state masked { meta.port = 3; transition accept; }
    state shadowed { meta.port = 4; transition accept; }
    state ranged { meta.port = 5; transition accept; }
    state other { meta.port = 6; transition accept; }
    // Projection leaves a nested tuple for the shared midend to flatten.
    state nested_dispatch {
        transition select(profile, {hdr.dispatch.kind, hdr.dispatch.kind}) {
            (1, {0xff, 0xff}): masked;
            default: other;
        }
    }
    state late_wildcard {
        // Copy propagation makes this key constant after early select simplification.
        enabled = true;
        transition select(hdr.dispatch.kind, enabled) {
            (_, true): masked;
            default: other;
        }
    }
}
control VerifyChecksumImpl(inout headers_t hdr, inout metadata_t meta) { apply { } }
control IngressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {
        if (sm.parser_error != error.NoError) {
            mark_to_drop(sm);
        } else {
            sm.egress_spec = meta.port;
        }
    }
}
control EgressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply { }
}
control ComputeChecksumImpl(inout headers_t hdr, inout metadata_t meta) { apply { } }
control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply { packet.emit(hdr.dispatch); }
}
V1Switch(ParserImpl(1), VerifyChecksumImpl(), IngressImpl(), EgressImpl(),
         ComputeChecksumImpl(), DeparserImpl()) main;
