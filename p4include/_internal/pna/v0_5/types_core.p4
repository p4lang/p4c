/**
 * These types need to be defined before including the architecture file
 * and the macro protecting them should be defined.
 */
#define PNA_PLACEHOLDER_CORE_TYPES
#ifdef PNA_PLACEHOLDER_CORE_TYPES

/* The bit widths shown below are placeholders that might not be
 * implemented by any PNA device.
 *
 * Each PNA implementation is free to use its own custom width in bits
 * for those types that are bit<W> for some W. */

/* These are defined using `typedef`, not `type`, so they are truly
 * just different names for the type bit<W> for the particular width W
 * shown.  Unlike the `type` definitions below, values declared with
 * the `typedef` type names can be freely mingled in expressions, just
 * as any value declared with type bit<W> can.  Values declared with
 * one of the `type` names below _cannot_ be so freely mingled, unless
 * you first cast them to the corresponding `typedef` type.  While
 * that may be inconvenient when you need to do arithmetic on such
 * values, it is the price to pay for having all occurrences of values
 * of the `type` types marked as such in the automatically generated
 * control plane API.
 *
 * Note that the width of typedef <name>Uint_t will always be the same
 * as the width of type <name>_t. */
typedef bit<32> PortIdUint_t;
typedef bit<32> InterfaceIdUint_t;
typedef bit<32> MulticastGroupUint_t;
typedef bit<16> MirrorSessionIdUint_t;
typedef bit<8>  MirrorSlotIdUint_t;
typedef bit<8>  ClassOfServiceUint_t;
typedef bit<16> PacketLengthUint_t;
typedef bit<16> MulticastInstanceUint_t;
typedef bit<64> TimestampUint_t;
typedef bit<32> FlowIdUint_t;
typedef bit<8>  ExpireTimeProfileIdUint_t;
typedef bit<3>  PassNumberUint_t;

typedef bit<32> SecurityAssocIdUint_t;

@p4runtime_translation("p4.org/pna/v1/PortId_t", 32)
type PortIdUint_t         PortId_t;
@p4runtime_translation("p4.org/pna/v1/InterfaceId_t", 32)
type InterfaceIdUint_t    InterfaceId_t;
@p4runtime_translation("p4.org/pna/v1/MulticastGroup_t", 32)
type MulticastGroupUint_t MulticastGroup_t;
@p4runtime_translation("p4.org/pna/v1/MirrorSessionId_t", 16)
type MirrorSessionIdUint_t MirrorSessionId_t;
@p4runtime_translation("p4.org/pna/v1/MirrorSlotId_t", 8)
type MirrorSlotIdUint_t MirrorSlotId_t;
@p4runtime_translation("p4.org/pna/v1/ClassOfService_t", 8)
type ClassOfServiceUint_t ClassOfService_t;
@p4runtime_translation("p4.org/pna/v1/PacketLength_t", 16)
type PacketLengthUint_t   PacketLength_t;
@p4runtime_translation("p4.org/pna/v1/MulticastInstance_t", 16)
type MulticastInstanceUint_t MulticastInstance_t;
@p4runtime_translation("p4.org/pna/v1/Timestamp_t", 64)
type TimestampUint_t      Timestamp_t;
@p4runtime_translation("p4.org/pna/v1/FlowId_t", 32)
type FlowIdUint_t      FlowId_t;
@p4runtime_translation("p4.org/pna/v1/ExpireTimeProfileId_t", 8)
type ExpireTimeProfileIdUint_t      ExpireTimeProfileId_t;
@p4runtime_translation("p4.org/pna/v1/PassNumber_t", 8)
type PassNumberUint_t      PassNumber_t;

@p4runtime_translation("p4.org/pna/v1/SecurityAssocId_t", 64)
type SecurityAssocIdUint_t      SecurityAssocId_t;

typedef error   ParserError_t;

const InterfaceId_t PNA_PORT_CPU = (InterfaceId_t) 0xfffffffd;

const MirrorSessionId_t PNA_MIRROR_SESSION_TO_CPU = (MirrorSessionId_t) 0;

#endif  // PNA_PLACEHOLDER_CORE_TYPES
