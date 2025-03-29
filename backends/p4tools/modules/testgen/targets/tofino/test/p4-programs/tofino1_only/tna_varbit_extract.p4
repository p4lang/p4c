#include <tna.p4>

header foo_t {
    varbit<32> foo;
}
struct headers {
    foo_t foo;
}
struct ingress_metadata_t {}
struct egress_headers_t {}
struct egress_metadata_t {}


parser ig_prs(
    packet_in pkt,
    out headers hdr,
    out ingress_metadata_t ig_md,
    out ingress_intrinsic_metadata_t ig_intr_md)
{
    state start {
        transition foo;
    }

    state foo {
        pkt.extract(hdr.foo, (bit<32>)8);
        transition accept;
    }
}

control ig_ctl(
    inout headers hdr, inout ingress_metadata_t ig_md,
    in ingress_intrinsic_metadata_t ig_intr_md,
    in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{
    apply {
    }
}

control ig_dprs(
    packet_out pkt,
    inout headers hdr,
    in ingress_metadata_t ig_md,
    in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md)
{
    apply {
    }
}

parser eg_prs(
    packet_in pkt,
    out egress_headers_t eg_hdr, out egress_metadata_t eg_md,
    out egress_intrinsic_metadata_t eg_intr_md)
{
    state start {
        transition accept;
    }
}

control eg_ctl(
    inout egress_headers_t eg_hdr, inout egress_metadata_t eg_md,
    in egress_intrinsic_metadata_t eg_intr_md,
    in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_oport_md)
{
    apply {
    }
}

control eg_dprs(
    packet_out pkt,
    inout egress_headers_t eg_hdr, in egress_metadata_t eg_md,
    in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md)
{
    apply {
    }
}
    
Pipeline(
    ig_prs(), ig_ctl(), ig_dprs(),
    eg_prs(), eg_ctl(), eg_dprs()) pipe;

Switch(pipe) main;
