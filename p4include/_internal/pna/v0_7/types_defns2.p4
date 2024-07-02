// BEGIN:Type_defns2

/* Note: All of the types with `InHeader` in their name are intended
 * only to carry values of the corresponding types in packet headers
 * between a PNA device and the P4Runtime Server software that manages
 * it.
 *
 * The widths are intended to be at least as large as any PNA device
 * will ever have for that type.  Thus these types may also be useful
 * to define packet headers that are sent directly between a PNA
 * device and other devices, without going through P4Runtime Server
 * software (e.g. this could be useful for sending packets to a
 * controller or data collection system using higher packet rates than
 * the P4Runtime Server can handle).  If used for this purpose, there
 * is no requirement that the PNA data plane _automatically_ perform
 * the numerical translation of these types that would occur if the
 * header went through the P4Runtime Server.  Any such desired
 * translation is up to the author of the P4 program to perform with
 * explicit code.
 *
 * All widths must be a multiple of 8, so that any subset of these
 * fields may be used in a single P4 header definition, even on P4
 * implementations that restrict headers to contain fields with a
 * total length that is a multiple of 8 bits. */

/* See the comments near the definition of PortIdUint_t for why these
 * typedef definitions exist. */
typedef bit<32> PortIdInHeaderUint_t;
typedef bit<32> InterfaceIdInHeaderUint_t;
typedef bit<32> MulticastGroupInHeaderUint_t;
typedef bit<16> MirrorSessionIdInHeaderUint_t;
typedef bit<8>  MirrorSlotIdInHeaderUint_t;
typedef bit<8>  ClassOfServiceInHeaderUint_t;
typedef bit<16> PacketLengthInHeaderUint_t;
typedef bit<16> MulticastInstanceInHeaderUint_t;
typedef bit<64> TimestampInHeaderUint_t;
typedef bit<32> FlowIdInHeaderUint_t;
typedef bit<8>  ExpireTimeProfileIdInHeaderUint_t;

typedef bit<32> SecurityAssocIdInHeaderUint_t;

@p4runtime_translation("p4.org/pna/v1/PortIdInHeader_t", 32)
type  PortIdInHeaderUint_t         PortIdInHeader_t;
@p4runtime_translation("p4.org/pna/v1/InterfaceIdInHeader_t", 32)
type  InterfaceIdInHeaderUint_t     InterfaceIdInHeader_t;
@p4runtime_translation("p4.org/pna/v1/MulticastGroupInHeader_t", 32)
type  MulticastGroupInHeaderUint_t MulticastGroupInHeader_t;
@p4runtime_translation("p4.org/pna/v1/MirrorSessionIdInHeader_t", 16)
type  MirrorSessionIdInHeaderUint_t MirrorSessionIdInHeader_t;
@p4runtime_translation("p4.org/pna/v1/MirrorSlotIdInHeader_t", 8)
type  MirrorSlotIdInHeaderUint_t MirrorSlotIdInHeader_t;
@p4runtime_translation("p4.org/pna/v1/ClassOfServiceInHeader_t", 8)
type  ClassOfServiceInHeaderUint_t ClassOfServiceInHeader_t;
@p4runtime_translation("p4.org/pna/v1/PacketLengthInHeader_t", 16)
type  PacketLengthInHeaderUint_t   PacketLengthInHeader_t;
@p4runtime_translation("p4.org/pna/v1/MulticastInstanceInHeader_t", 16)
type  MulticastInstanceInHeaderUint_t MulticastInstanceInHeader_t;
@p4runtime_translation("p4.org/pna/v1/TimestampInHeader_t", 64)
type  TimestampInHeaderUint_t      TimestampInHeader_t;
@p4runtime_translation("p4.org/pna/v1/FlowIdInHeader_t", 32)
type  FlowIdInHeaderUint_t      FlowIdInHeader_t;
@p4runtime_translation("p4.org/pna/v1/ExpireTimeProfileIdInHeader_t", 8)
type  ExpireTimeProfileIdInHeaderUint_t      ExpireTimeProfileIdInHeader_t;

@p4runtime_translation("p4.org/pna/v1/SecurityAssocIdInHeader_t", 64)
type  SecurityAssocIdInHeaderUint_t      SecurityAssocIdInHeader_t;
// END:Type_defns2
