#ifndef _TOFINO5_NATIVE_ARCHITECTURE_P4_
#define _TOFINO5_NATIVE_ARCHITECTURE_P4_

#include "core.p4"
#include "tofino5.p4"

// The following declarations provide a template for the programmable blocks in
// Tofino5.
parser IngressParserT<H, M>(
    packet_in pkt,
    out H hdr,
    out M ig_md,
    out ingress_intrinsic_metadata_t ig_intr_md,
    @optional out ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm);

control IngressT<H, M>(
    in H hdr,
    inout M ig_md,
    @optional in ingress_intrinsic_metadata_t ig_intr_md,
    @optional inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm);

control EgressT<H, M>(
    inout H hdr,
    inout M eg_md,
    @optional in egress_intrinsic_metadata_t eg_intr_md,
    @optional inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
    @optional inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport);

control EgressDeparserT<H, M>(
    packet_out pkt,
    inout H hdr,
    in M metadata,
    @optional in egress_intrinsic_metadata_t eg_intr_md,
    @optional in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr);

// TODO: once ghost thread support is added here, it needs to be also added into
// the CollectPipeline pass, namely the CollectPipelines::Pipe::set isFlatrock
// `if` (bf-p4c/midend/collect_pipelines.cpp) to detect it properly.
package Pipeline<H, M>(
    IngressParserT<H, M> ingress_parser,
    IngressT<H, M> ingress,
    EgressT<H, M> egress,
    EgressDeparserT<H, M> egress_deparser);

#define PIPELINE_TYPE_ARGS(n) H##n, M##n
#define PIPELINE(n) @optional Pipeline<H##n, M##n> pipe##n

@pkginfo(arch="T5NA", version="0.1.0")
package Switch<PIPELINE_TYPE_ARGS(0), PIPELINE_TYPE_ARGS(1), PIPELINE_TYPE_ARGS(2), PIPELINE_TYPE_ARGS(3),
               PIPELINE_TYPE_ARGS(4), PIPELINE_TYPE_ARGS(5), PIPELINE_TYPE_ARGS(6), PIPELINE_TYPE_ARGS(7),
               PIPELINE_TYPE_ARGS(8), PIPELINE_TYPE_ARGS(9), PIPELINE_TYPE_ARGS(10), PIPELINE_TYPE_ARGS(11),
               PIPELINE_TYPE_ARGS(12), PIPELINE_TYPE_ARGS(13), PIPELINE_TYPE_ARGS(14), PIPELINE_TYPE_ARGS(15)>
    (PIPELINE(0), PIPELINE(1), PIPELINE(2), PIPELINE(3), PIPELINE(4), PIPELINE(5), PIPELINE(6), PIPELINE(7),
     PIPELINE(8), PIPELINE(9), PIPELINE(10), PIPELINE(11), PIPELINE(12), PIPELINE(13), PIPELINE(14), PIPELINE(15));

#endif  /* _TOFINO5_NATIVE_ARCHITECTURE_P4_ */
