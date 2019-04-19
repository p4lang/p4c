#include <v1model.p4>

struct headers_t { }

struct metadata {
    bool cond;
    bit<8> field;
}

parser ParserImpl (
    packet_in buffer,
    out headers_t parsed_hdr,
    inout metadata meta,
    inout standard_metadata_t ostd
    )
{ state start { transition accept; } }

control VerifyChecksumImpl (
    inout headers_t hdr,
    inout metadata meta
    )
{ apply { } }

control IngressImpl (
    inout headers_t hdr,
    inout metadata meta,
    inout standard_metadata_t ostd
    )
{ apply { } }

control EgressImpl (
    inout headers_t hdr,
    inout metadata meta,
    inout standard_metadata_t ostd
    )
{
    action set_true() {
        if (meta.field == 0) {
            meta.cond = true;
        }
    }

    table change_cond {
        key = { ostd.egress_spec: exact; }
        actions = { set_true; }
    }

    apply {
        change_cond.apply();
    }
}

control ComputeChecksumImpl (
    inout headers_t hdr,
    inout metadata meta)
{ apply { } }

control DeparserImpl (
    packet_out buffer,
    in headers_t hdr)
{ apply { } }

V1Switch(
    ParserImpl(),
    VerifyChecksumImpl(),
    IngressImpl(),
    EgressImpl(),
    ComputeChecksumImpl(),
    DeparserImpl()
) main;
