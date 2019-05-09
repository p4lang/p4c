#include "core.p4"

// From psa.p4

#define PORT_BITWIDTH 9  // target-specific

// PortIdUInt_t is only intended for casting values from type PortId_t
// to an unsigned integer with the same width.
typedef bit<PORT_BITWIDTH> PortIdUInt_t;

// request translation by P4Runtime agent of all occurrences of PortId_t exposed
// to control-plane
type bit<PORT_BITWIDTH> PortId_t;

//const PortId_t PSA_CPU_PORT = (PortId_t) 9w192; // target-specific
// With (PortId_t) cast on unsigned int literal, I get this error from
// type branch of p4c:
//psa-portid-using-type1.p4(16): error: cast: Cannot evaluate initializer for constant

//const PortId_t PSA_CPU_PORT = 9w192;  // target-specific
// With no cast on unsigned int literal, I get this error:
//psa-portid-using-type1.p4(14): error: PSA_CPU_PORT: Cannot unify bit<9> to PortId_t
//type bit<9> PortId_t;
//                        ^
//psa-portid-using-type1.p4(14)
//type bit<9> PortId_t;
//               ^^^^^^^^

// This also gives 'Cannot evaluate initializer for constant'.
//const PortId_t PSA_CPU_PORT = ((PortId_t) ((bit<9>) 9w192));  // target-specific

// This is intended as a workaround until 'const' can be used for
// PSA_CPU_PORT, or I suppose it could be a long term solution, too.
#define PSA_CPU_PORT ((PortId_t) 9w192)

// The width of this is _not_ target-specific.  All targets will use
// this bit width.  This type is only intended for copying data-plane
// number space PortId_t values into headers to be sent to the CPU
// port, or for parsing headers containing PortId_t values in certain
// headers in packets from the CPU port.
type bit<32> PortIdInHeader_t;

// Note: all of these casts are necessary with the current type
// code.  Eliminating any one of them results in a compiler error when
// using the macro as done later in this program.  The nice thing is
// that this macro could be added to psa.p4, and most people won't
// need to see it.

#define PSA_PORT_HEADER_TO_ID(p) ((PortId_t) (PortIdUInt_t) (bit<32>) (p))
#define PSA_PORT_ID_TO_HEADER(p) ((PortIdInHeader_t) (bit<32>) (PortIdUInt_t) (p))

// Included here simply so I can use type PortId_t in my
// locally-modified version of v1model.p4, called v1model-type.p4.
#include "v1model-newtype.pp"


header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}


// From fabric.p4
@controller_header("packet_in")
header packet_in_header_t {
    // Translation will occur in agent software for this field,
    // because it is inside of a controller_header, and because
    // PortIdInHeader_t is defined as a type with a
    PortIdInHeader_t ingress_port;
}

@controller_header("packet_out")
header packet_out_header_t {
    // No annotation needed here to get agent translation of
    // egress_port, for same reason as in header packet_in_header_t
    // above.
    PortIdInHeader_t egress_port;
}


struct parsed_headers_t {
    packet_in_header_t packet_in;
    packet_out_header_t packet_out;
    ethernet_t ethernet;
    ipv4_t ipv4;
}

struct fabric_metadata_t {
}

action drop() {
    mark_to_drop();
}

action nop() {
}

parser FabricParser(packet_in packet,
                    out parsed_headers_t hdr,
                    inout fabric_metadata_t meta,
                    inout standard_metadata_t standard_metadata)
{
    state start {
        transition accept;
    }
}

control PacketIoIngress(inout parsed_headers_t hdr,
                        inout fabric_metadata_t fabric_metadata,
                        inout standard_metadata_t standard_metadata)
{
    apply {
        if (hdr.packet_out.isValid()) {
            // I know what I'm doing, I requested translation on
            // packet_out.egress_port
            standard_metadata.egress_spec =
                PSA_PORT_HEADER_TO_ID(hdr.packet_out.egress_port);
            hdr.packet_out.setInvalid();
            exit;
        }
    }
}

control PacketIoEgress(inout parsed_headers_t hdr,
                       inout fabric_metadata_t fabric_metadata,
                       inout standard_metadata_t standard_metadata)
{
    apply {
        // comparison of PortId_t to PortId_t is valid
        if (standard_metadata.egress_port == PSA_CPU_PORT) {
            hdr.packet_in.setValid();
            // I know what I'm doing, I requested translation on
            // packet_in.ingress_port
            hdr.packet_in.ingress_port =
                PSA_PORT_ID_TO_HEADER(standard_metadata.ingress_port);
        }
    }
}

control Filtering (inout parsed_headers_t hdr,
                   inout fabric_metadata_t fabric_metadata,
                   inout standard_metadata_t standard_metadata)
{
    table t {
        key = {
            // translated by P4Runtime agent because of annotation on PortId_t
            standard_metadata.ingress_port: exact;
        }
        actions = { drop; nop; }
    }

    apply { t.apply(); }
}

control Forwarding (inout parsed_headers_t hdr,
                    inout fabric_metadata_t fabric_metadata,
                    inout standard_metadata_t standard_metadata)
{
    // next_port translated by P4Runtime agent because of annotation on PortId_t
    action fwd(PortId_t next_port) {
        standard_metadata.egress_spec = next_port;
    }

    table t {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = { drop; fwd; }
    }

    apply {
        t.apply();

        // One can perform arithmetic operations on PortId_t values if
        // you use enough casts to convert it to an unsigned int, do
        // the arithetic ops, then cast back to PortId_t.

        // compiler error expected because + cannot be used with an
        // operand of type PortId_t
        //standard_metadata.egress_spec = standard_metadata.egress_spec + 1;

        // compiler ok, but error message is fairly confusing if you
        // misspell a type name PortIdUInt_t as the slightly wrong
        // PortIdUint_t
        PortIdUInt_t tmp2 = 1;

        // compiler error which looks reasonable for following line:
//sa-portid-using-type2.p4(207): error: AssignmentStatement: Cannot unify bit<9> to PortId_t
//        standard_metadata.egress_spec = ((PortIdUInt_t) standard_metadata.egress_spec) + 1;
//                                      ^
//psa-portid-using-type2.p4(14)
//type bit<9> PortId_t;
//               ^^^^^^^^
        //standard_metadata.egress_spec = ((PortIdUInt_t) standard_metadata.egress_spec) + 1;

        // compiler ok
        standard_metadata.egress_spec =
            (PortId_t) (((PortIdUInt_t) standard_metadata.egress_spec) + 1);

        // compiler ok
        PortId_t mask = (PortId_t) ((PortIdUInt_t) 0xf);

        // compiler error - expected because trying to & with mask
        // that has type PortId_t
//        standard_metadata.egress_spec =
//            (PortId_t) (((PortIdUInt_t) standard_metadata.egress_spec) &
//                        mask);

        // compiler ok
        standard_metadata.egress_spec =
            (PortId_t) (((PortIdUInt_t) standard_metadata.egress_spec) &
                        ((PortIdUInt_t) mask));
    }
}

control FabricIngress (inout parsed_headers_t hdr,
                       inout fabric_metadata_t fabric_metadata,
                       inout standard_metadata_t standard_metadata)
{
    PacketIoIngress() packet_io_ingress;
    Filtering() filtering;
    Forwarding() forwarding;

    apply {
        packet_io_ingress.apply(hdr, fabric_metadata, standard_metadata);
        filtering.apply(hdr, fabric_metadata, standard_metadata);
        forwarding.apply(hdr, fabric_metadata, standard_metadata);
    }
}

control FabricEgress (inout parsed_headers_t hdr,
                      inout fabric_metadata_t fabric_metadata,
                      inout standard_metadata_t standard_metadata)
{
    PacketIoEgress() pkt_io_egress;
    apply {
        pkt_io_egress.apply(hdr, fabric_metadata, standard_metadata);
    }
}

control FabricDeparser(packet_out packet,
                       in parsed_headers_t hdr)
{
    apply {
    }
}

control FabricVerifyChecksum(inout parsed_headers_t hdr,
                             inout fabric_metadata_t meta)
{
    apply {
    }
}

control FabricComputeChecksum(inout parsed_headers_t hdr,
                              inout fabric_metadata_t meta)
{
    apply {
    }
}

V1Switch(
    FabricParser(),
    FabricVerifyChecksum(),
    FabricIngress(),
    FabricEgress(),
    FabricComputeChecksum(),
    FabricDeparser()
) main;
