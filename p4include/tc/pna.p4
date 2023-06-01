/* Copyright 2020-present Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef __PNA_P4__
#define __PNA_P4__

#include<core.p4>

/**
 *   P4-16 declaration of the Portable NIC Architecture
 */

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

@p4runtime_translation("p4.org/pna/v1/SecurityAssocId_t", 64)
type SecurityAssocIdUint_t      SecurityAssocId_t;

typedef error   ParserError_t;

const InterfaceId_t PNA_PORT_CPU = (InterfaceId_t) 0xfffffffd;

const MirrorSessionId_t PNA_MIRROR_SESSION_TO_CPU = (MirrorSessionId_t) 0;

#endif  // PNA_PLACEHOLDER_CORE_TYPES

#ifndef PNA_PLACEHOLDER_CORE_TYPES
#error "Please define the following types for PNA and undef the PNA_PLACEHOLDER_CORE_TYPES macro"
// BEGIN:Type_defns
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
typedef bit<unspecified> PortIdUint_t;
typedef bit<unspecified> InterfaceIdUint_t;
typedef bit<unspecified> MulticastGroupUint_t;
typedef bit<unspecified> MirrorSessionIdUint_t;
typedef bit<unspecified> MirrorSlotIdUint_t;
typedef bit<unspecified> ClassOfServiceUint_t;
typedef bit<unspecified> PacketLengthUint_t;
typedef bit<unspecified> MulticastInstanceUint_t;
typedef bit<unspecified> TimestampUint_t;
typedef bit<unspecified> FlowIdUint_t;
typedef bit<unspecified> ExpireTimeProfileIdUint_t;

typedef bit<unspecified> SecurityAssocIdUint_t;

@p4runtime_translation("p4.org/pna/v1/PortId_t", 32)
type PortIdUint_t         PortId_t;
@p4runtime_translation("p4.org/pna/v1/InterfaceId_t", 32)
type InterfaceIdUint_t     InterfaceId_t;
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

@p4runtime_translation("p4.org/pna/v1/SecurityAssocId_t", 64)
type SecurityAssocIdUint_t      SecurityAssocId_t;

typedef error   ParserError_t;

const InterfaceId_t PNA_PORT_CPU = (PortId_t) unspecified;

const MirrorSessionId_t PNA_MIRROR_SESSION_TO_CPU = (MirrorSessiontId_t) unspecified;
// END:Type_defns
#endif  // #ifndef PNA_EXAMPLE_CORE_TYPES

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

/* The _int_to_header functions were written to convert a value of
 * type <name>_t (a value INTernal to the data path) to a value of
 * type <name>InHeader_t inside a header that will be sent to the CPU
 * port.
 *
 * The _header_to_int functions were written to convert values in the
 * opposite direction, typically for assigning a value in a header
 * received from the CPU port, to a value you wish to use in the rest
 * of your code.
 *
 * The reason that three casts are needed is that each of the original
 * and target types is declared via P4_16 'type', so without a cast
 * they can only be assigned to values of that identical type.  The
 * first cast changes it from the original 'type' to a 'bit<W1>' value
 * of the same bit width W1.  The second cast changes its bit width,
 * either prepending 0s if it becomes wider, or discarding the most
 * significant bits if it becomes narrower.  The third cast changes it
 * from a 'bit<W2>' value to the final 'type', with the same width
 * W2. */

PortId_t pna_PortId_header_to_int (in PortIdInHeader_t x) {
    return (PortId_t) (PortIdUint_t) (PortIdInHeaderUint_t) x;
}
InterfaceId_t pna_InterfaceId_header_to_int (in InterfaceIdInHeader_t x) {
    return (InterfaceId_t) (InterfaceIdUint_t) (InterfaceIdInHeaderUint_t) x;
}
MulticastGroup_t pna_MulticastGroup_header_to_int (in MulticastGroupInHeader_t x) {
    return (MulticastGroup_t) (MulticastGroupUint_t) (MulticastGroupInHeaderUint_t) x;
}
MirrorSessionId_t pna_MirrorSessionId_header_to_int (in MirrorSessionIdInHeader_t x) {
    return (MirrorSessionId_t) (MirrorSessionIdUint_t) (MirrorSessionIdInHeaderUint_t) x;
}
ClassOfService_t pna_ClassOfService_header_to_int (in ClassOfServiceInHeader_t x) {
    return (ClassOfService_t) (ClassOfServiceUint_t) (ClassOfServiceInHeaderUint_t) x;
}
PacketLength_t pna_PacketLength_header_to_int (in PacketLengthInHeader_t x) {
    return (PacketLength_t) (PacketLengthUint_t) (PacketLengthInHeaderUint_t) x;
}
MulticastInstance_t pna_MulticastInstance_header_to_int (in MulticastInstanceInHeader_t x) {
    return (MulticastInstance_t) (MulticastInstanceUint_t) (MulticastInstanceInHeaderUint_t) x;
}
Timestamp_t pna_Timestamp_header_to_int (in TimestampInHeader_t x) {
    return (Timestamp_t) (TimestampUint_t) (TimestampInHeaderUint_t) x;
}
FlowId_t pna_FlowId_header_to_int (in FlowIdInHeader_t x) {
    return (FlowId_t) (FlowIdUint_t) (FlowIdInHeaderUint_t) x;
}
ExpireTimeProfileId_t pna_ExpireTimeProfileId_header_to_int (in ExpireTimeProfileIdInHeader_t x) {
    return (ExpireTimeProfileId_t) (ExpireTimeProfileIdUint_t) (ExpireTimeProfileIdInHeaderUint_t) x;
}

PortIdInHeader_t pna_PortId_int_to_header (in PortId_t x) {
    return (PortIdInHeader_t) (PortIdInHeaderUint_t) (PortIdUint_t) x;
}
InterfaceIdInHeader_t pna_InterfaceId_int_to_header (in InterfaceId_t x) {
    return (InterfaceIdInHeader_t) (InterfaceIdInHeaderUint_t) (InterfaceIdUint_t) x;
}
MulticastGroupInHeader_t pna_MulticastGroup_int_to_header (in MulticastGroup_t x) {
    return (MulticastGroupInHeader_t) (MulticastGroupInHeaderUint_t) (MulticastGroupUint_t) x;
}
MirrorSessionIdInHeader_t pna_MirrorSessionId_int_to_header (in MirrorSessionId_t x) {
    return (MirrorSessionIdInHeader_t) (MirrorSessionIdInHeaderUint_t) (MirrorSessionIdUint_t) x;
}
ClassOfServiceInHeader_t pna_ClassOfService_int_to_header (in ClassOfService_t x) {
    return (ClassOfServiceInHeader_t) (ClassOfServiceInHeaderUint_t) (ClassOfServiceUint_t) x;
}
PacketLengthInHeader_t pna_PacketLength_int_to_header (in PacketLength_t x) {
    return (PacketLengthInHeader_t) (PacketLengthInHeaderUint_t) (PacketLengthUint_t) x;
}
MulticastInstanceInHeader_t pna_MulticastInstance_int_to_header (in MulticastInstance_t x) {
    return (MulticastInstanceInHeader_t) (MulticastInstanceInHeaderUint_t) (MulticastInstanceUint_t) x;
}
TimestampInHeader_t pna_Timestamp_int_to_header (in Timestamp_t x) {
    return (TimestampInHeader_t) (TimestampInHeaderUint_t) (TimestampUint_t) x;
}
FlowIdInHeader_t pna_FlowId_int_to_header (in FlowId_t x) {
    return (FlowIdInHeader_t) (FlowIdInHeaderUint_t) (FlowIdUint_t) x;
}
ExpireTimeProfileIdInHeader_t pna_ExpireTimeProfileId_int_to_header (in ExpireTimeProfileId_t x) {
    return (ExpireTimeProfileIdInHeader_t) (ExpireTimeProfileIdInHeaderUint_t) (ExpireTimeProfileIdUint_t) x;
}

// BEGIN:enum_PNA_IdleTimeout_t
/// Supported values for the pna_idle_timeout table property
enum PNA_IdleTimeout_t {
    NO_TIMEOUT,
    NOTIFY_CONTROL,
    AUTO_DELETE
};
// END:enum_PNA_IdleTimeout_t

// BEGIN:Match_kinds
match_kind {
    range,   /// Used to represent min..max intervals
    selector, /// Used for dynamic action selection via the ActionSelector extern
    optional /// Either an exact match, or a wildcard matching any value for the entire field
}
// END:Match_kinds

// BEGIN:Hash_algorithms
enum PNA_HashAlgorithm_t {
  IDENTITY,
  CRC32,
  CRC32_CUSTOM,
  CRC16,
  CRC16_CUSTOM,
  ONES_COMPLEMENT16,  /// One's complement 16-bit sum used for IPv4 headers,
                      /// TCP, and UDP.
  TARGET_DEFAULT      /// target implementation defined
}
// END:Hash_algorithms

// BEGIN:Hash_extern
extern Hash<O> {
  /// Constructor
  Hash(PNA_HashAlgorithm_t algo);

  /// Compute the hash for data.
  /// @param data The data over which to calculate the hash.
  /// @return The hash value.
  O get_hash<D>(in D data);

  /// Compute the hash for data, with modulo by max, then add base.
  /// @param base Minimum return value.
  /// @param data The data over which to calculate the hash.
  /// @param max The hash value is divided by max to get modulo.
  ///        An implementation may limit the largest value supported,
  ///        e.g. to a value like 32, or 256, and may also only
  ///        support powers of 2 for this value.  P4 developers should
  ///        limit their choice to such values if they wish to
  ///        maximize portability.
  /// @return (base + (h % max)) where h is the hash value.
  O get_hash<T, D>(in T base, in D data, in T max);
}
// END:Hash_extern

// BEGIN:Checksum_extern
extern Checksum<W> {
  /// Constructor
  Checksum(PNA_HashAlgorithm_t hash);

  /// Reset internal state and prepare unit for computation.
  /// Every instance of a Checksum object is automatically initialized as
  /// if clear() had been called on it. This initialization happens every
  /// time the object is instantiated, that is, whenever the parser or control
  /// containing the Checksum object are applied.
  /// All state maintained by the Checksum object is independent per packet.
  void clear();

  /// Add data to checksum
  void update<T>(in T data);

  /// Get checksum for data added (and not removed) since last clear
  W    get();
}
// END:Checksum_extern

// BEGIN:InternetChecksum_extern
// Checksum based on `ONES_COMPLEMENT16` algorithm used in IPv4, TCP, and UDP.
// Supports incremental updating via `subtract` method.
// See IETF RFC 1624.
extern InternetChecksum {
  /// Constructor
  InternetChecksum();

  /// Reset internal state and prepare unit for computation.  Every
  /// instance of an InternetChecksum object is automatically
  /// initialized as if clear() had been called on it, once for each
  /// time the parser or control it is instantiated within is
  /// executed.  All state maintained by it is independent per packet.
  void clear();

  /// Add data to checksum.  data must be a multiple of 16 bits long.
  void add<T>(in T data);

  /// Subtract data from existing checksum.  data must be a multiple of
  /// 16 bits long.
  void subtract<T>(in T data);

  /// Get checksum for data added (and not removed) since last clear
  bit<16> get();

  /// Get current state of checksum computation.  The return value is
  /// only intended to be used for a future call to the set_state
  /// method.
  bit<16> get_state();

  /// Restore the state of the InternetChecksum instance to one
  /// returned from an earlier call to the get_state method.  This
  /// state could have been returned from the same instance of the
  /// InternetChecksum extern, or a different one.
  void set_state(in bit<16> checksum_state);
}
// END:InternetChecksum_extern

// BEGIN:CounterType_defn
enum PNA_CounterType_t {
    PACKETS,
    BYTES,
    PACKETS_AND_BYTES
}
// END:CounterType_defn

// BEGIN:Counter_extern
/// Indirect counter with n_counters independent counter values, where
/// every counter value has a data plane size specified by type W.

@noWarn("unused")
extern Counter<W, S> {
  Counter(bit<32> n_counters, PNA_CounterType_t type);
  void count(in S index);
}
// END:Counter_extern

// BEGIN:DirectCounter_extern
@noWarn("unused")
extern DirectCounter<W> {
  DirectCounter(PNA_CounterType_t type);
  void count();
}
// END:DirectCounter_extern

// BEGIN:MeterType_defn
enum PNA_MeterType_t {
    PACKETS,
    BYTES
}
// END:MeterType_defn

// BEGIN:MeterColor_defn
enum PNA_MeterColor_t { RED, GREEN, YELLOW }
// END:MeterColor_defn

// BEGIN:Meter_extern
// Indexed meter with n_meters independent meter states.

extern Meter<S> {
  Meter(bit<32> n_meters, PNA_MeterType_t type);

  // Use this method call to perform a color aware meter update (see
  // RFC 2698). The color of the packet before the method call was
  // made is specified by the color parameter.
  PNA_MeterColor_t execute(in S index, in PNA_MeterColor_t color);

  // Use this method call to perform a color blind meter update (see
  // RFC 2698).  It may be implemented via a call to execute(index,
  // MeterColor_t.GREEN), which has the same behavior.
  PNA_MeterColor_t execute(in S index);
}
// END:Meter_extern

// BEGIN:DirectMeter_extern
extern DirectMeter {
  DirectMeter(PNA_MeterType_t type);
  // See the corresponding methods for extern Meter.
  PNA_MeterColor_t execute(in PNA_MeterColor_t color);
  PNA_MeterColor_t execute();
}
// END:DirectMeter_extern

// BEGIN:Register_extern
extern Register<T, S> {
  /// Instantiate an array of <size> registers. The initial value is
  /// undefined.
  Register(bit<32> size);
  /// Initialize an array of <size> registers and set their value to
  /// initial_value.
  Register(bit<32> size, T initial_value);

  T    read  (in S index);
  void write (in S index, in T value);
}
// END:Register_extern

// BEGIN:Random_extern
extern Random<T> {

  /// Return a random value in the range [min, max], inclusive.
  /// Implementations are allowed to support only ranges where (max -
  /// min + 1) is a power of 2.  P4 developers should limit their
  /// arguments to such values if they wish to maximize portability.

  Random(T min, T max);
  T read();
}
// END:Random_extern

// BEGIN:ActionProfile_extern
extern ActionProfile {
  /// Construct an action profile of 'size' entries
  ActionProfile(bit<32> size);
}
// END:ActionProfile_extern

// BEGIN:ActionSelector_extern
extern ActionSelector {
  /// Construct an action selector of 'size' entries
  /// @param algo hash algorithm to select a member in a group
  /// @param size number of entries in the action selector
  /// @param outputWidth size of the key
  ActionSelector(PNA_HashAlgorithm_t algo, bit<32> size, bit<32> outputWidth);
}
// END:ActionSelector_extern

// BEGIN:Digest_extern
extern Digest<T> {
  Digest();                       /// define a digest stream to the control plane
  void pack(in T data);           /// emit data into the stream
}
// END:Digest_extern

enum PNA_Source_t {
    FROM_HOST,
    FROM_NET
}

// BEGIN:Metadata_types

struct pna_main_parser_input_metadata_t {
    // common fields initialized for all packets that are input to main
    // parser, regardless of direction.
    bool                     recirculated;
    // If this packet has FROM_NET source, input_port contains
    // the id of the network port on which the packet arrived.
    // If this packet has FROM_HOST source, input_port contains
    // the id of the vport from which the packet came
    PortId_t                 input_port;   // network/host port id
}

// is_host_port(p) returns true if p is a host port, otherwise false.
extern bool is_host_port (in PortId_t p);

// is_net_port(p) returns true if p is a network port, otherwise
// false.
extern bool is_net_port (in PortId_t p);

struct pna_main_input_metadata_t {
    // common fields initialized for all packets that are input to main
    // parser, regardless of direction.
    bool                     recirculated;
    Timestamp_t              timestamp;
    ParserError_t            parser_error;
    ClassOfService_t         class_of_service;
    // See comments for field input_port in struct
    // pna_main_parser_input_metadata_t
    PortId_t                 input_port;
}

// BEGIN:Metadata_main_output
struct pna_main_output_metadata_t {
  // common fields used by the architecture to decide what to do with
  // the packet next, after the main parser, control, and deparser
  // have finished executing one pass, regardless of the direction.
  ClassOfService_t         class_of_service; // 0
}
// END:Metadata_main_output
// END:Metadata_types


// The following extern functions are "forwarding" functions -- they
// all set the destination of the packet.  Calling one of them
// overwrites and replaces the effect of any earlier call to any of
// the functions in this set.  Only the last one executed will
// actually take effect for the packet.

// + drop_packet
// + send_to_port


// drop_packet() - Cause the packet to be dropped when it finishes
// completing the main control.
//
// Invoking drop_packet() is supported only within the main control.

extern void drop_packet();

// BEGIN:send_to_port
extern void send_to_port(in PortId_t dest_port);
// END:send_to_port

// BEGIN:mirror_packet
extern void mirror_packet(in MirrorSlotId_t mirror_slot_id,
                          in MirrorSessionId_t mirror_session_id);
// END:mirror_packet

// TBD: Does it make sense to have a data plane add of a hit action
// that has in, out, or inout parameters?
//
// TBD: Should we require the return value?  Can most targets
// implement it?  If not, consider having two separate variants of
// add_entry, one with no return value (i.e. type void).  Such a
// variant of add_entry seems difficult to use correctly, if it is
// possible for entries to fail to be added.

// BEGIN:add_entry_extern_function
// The bit width of this type is allowed to be different for different
// target devices.  It must be at least a 1-bit wide type.

typedef bit<1> AddEntryErrorStatus_t;

const AddEntryErrorStatus_t ADD_ENTRY_SUCCESS = 0;
const AddEntryErrorStatus_t ADD_ENTRY_NOT_DONE = 1;

// Targets may define target-specific non-0 constants of type
// AddEntryErrorStatus_t if they wish.

// The add_entry() extern function causes an entry, i.e. a key and its
// corresponding action and action parameter values, to be added to a
// table from the data plane, i.e. without the control plane having to
// take any action at all to cause the table entry to be added.
//
// The key of the new entry added will always be the same as the key
// that was just looked up in the table, and experienced a miss.
//
// `action_name` is the name of an action that must satisfy these
// restrictions:
// + It must be an action that is in the list specified as the
//   `actions` property of the table.
// + It must be possible for this action to be the action of an entry
//   added to the table, e.g. it is an error if the action has the
//   annotation `@defaultonly`.
// + The action to be added must not itself contain a call to
//   add_entry(), or anything else that is not supported in a table's
//   "hit action".
//
// Type T must be a struct type whose field names have the same name
// as the parameters of the action being added, in the same order, and
// have the same type as the corresponding action parameters.
//
// `action_params` will become the action parameters of the new entry
// to be added.
//
// `expire_time_profile_id` is the initial expire time profile id of
// the entry added.
//
// The return value will be ADD_ENTRY_SUCCESS if the entry was
// successfully added, otherwise it will be some other value not equal
// to ADD_ENTRY_SUCCESS.  Targets are allowed to define only one
// failure return value, or several if they wish to provide more
// detail on the reason for the failure to add the entry.
//
// It is NOT defined by PNA, and need not be supported by PNA
// implementations, to call add_entry() within an action that is added
// as an entry of a table, i.e. as a "hit action".  It is only defined
// if called within an action that is the default_action, i.e. a "miss
// action" of a table.
//
// For tables with `add_on_miss = true`, some PNA implementations
// might only support `default_action` with the `const` qualifier.
// However, if a PNA implementation can support run-time modifiable
// default actions for such a table, some of which call add_entry()
// and some of which do not, the behavior of such an implementation is
// defined by PNA, and this may be a useful feature.

extern AddEntryErrorStatus_t add_entry<T>(
    string action_name,
    in T action_params,
    in ExpireTimeProfileId_t expire_time_profile_id);
// END:add_entry_extern_function

// The following call to add_entry_if():
//
//     add_entry_if(expr, action_name, action_params, expire_time_profile_id);
//
// has exactly the same behavior as the following expression:
//
//     (expr) ? add_entry(action_name, action_params, expire_time_profile_id)
//            : ADD_ENTRY_NOT_DONE;
//
// and it has the same restrictions on where it can appear in a P4
// program as that equivalent code.
//
// Rationale: At the time PNA was being designed in 2022, there were
// P4 targets, including the BMv2 software switch in the repository
// https://github.com/p4lang/behavioral-model, that did not fully
// support `if` statements within P4 actions.  add_entry_if() enables
// writing P4 code without `if` statements within P4 actions that
// would otherwise require an `if` statement to express the desired
// behavior.  Admittedly, this is a work-around for targets with
// limited support for `if` statements within P4 actions.  See
// https://github.com/p4lang/pna/issues/63 for more details.

extern AddEntryErrorStatus_t add_entry_if<T>(
    in bool do_add_entry,
    string action_name,
    in T action_params,
    in ExpireTimeProfileId_t expire_time_profile_id);

extern FlowId_t allocate_flow_id();


// set_entry_expire_time() may only be called from within an action of
// a table with property 'pna_idle_timeout' having a value of
// `NOTIFY_CONTROL` or `AUTO_DELETE`.

// Calling it causes the expiration time profile id of the matched
// entry to become equal to expire_time_profile_id.

// It _also_ behaves as if restart_expire_timer() was called.

extern void set_entry_expire_time(
    in ExpireTimeProfileId_t expire_time_profile_id);


// restart_expire_timer() may only be called from within an action of
// a table with property 'pna_idle_timeout' having a value of
// `NOTIFY_CONTROL` or `AUTO_DELETE`.

// Calling it causes the dynamic expiration timer of the entry to be
// reset, so that the entry will remain active from the now until that
// time in the future.

// TODO: Note that there are targets that support a table with
// property pna_idle_timeout equal to `NOTIFY_CONTROL` or
// `AUTO_DELETE` such that matching an entry _does not_ cause this
// side effect to occur, i.e. it is possible to match an entry and
// _not_ do what `restart_expire_timer()` does.  We should document
// this explicitly as common for PNA devices, if that is agreed upon.
// Andy is pretty sure that PSA devices were not expected to have this
// option.

// Note that for the PSA architecture with psa_idle_timeout =
// PSA_IdleTimeout_t.NOTIFY_CONTROL, I believe there was an implicit
// assumption that _every_ time a table entry was matched, the target
// behaved as if restart_expire_timer() was called, but there was no
// such extern function defined by PSA.

// The proposal for PNA is that it is possible to match an entry, but
// _not_ call restart_expire_timer(), and this will cause the data
// plane _not_ to restart the table entry's expiration time.  That is,
// the expiration time of the entry will continue to be the same as
// before it was matched.

extern void restart_expire_timer();

// The effect of this call:
//
//     update_expire_info(a, b, c)
//
// is exactly the same as the effect of the following code:
//
//    if (a) {
//        if (b) {
//            set_entry_expire_time(c);
//        } else {
//            restart_expire_timer();
//        }
//    }

extern void update_expire_info(
    in bool update_aging_info,
    in bool update_expire_time,
    in ExpireTimeProfileId_t expire_time_profile_id);


// Set the expire time of the matched entry in the table to the value
// specified in the parameter expire_time_profile_id, if condition
// in the first parameter evaluates to true. Otherwise, the function
// has no effect.
//
// @param condition  The boolean expression to evaluate to determine
//                   if the expire time will be set.
// @param expire_time_profile_id
//                   The expire time to set for the matched entry,
//                   if the data and value parameters are equal.
//
// Examples:
// set_entry_expire_time_if(hdr.tcp.flags == TCP_FLG_SYN &&
//                          meta.direction == OUTBOUND,
//                          tcp_connection_start_time_profile_id);
// set_entry_expire_time_if(hdr.tcp.flags == TCP_FLG_ACK,
//                          tcp_connection_continuation_time_protile_id);
// set_entry_expire_time_if(hdr.tcp.flags == TCP_FLG_FIN,
//                          tcp_connection_close_time_profile_id);

extern void set_entry_expire_time_if(
    in bool condition,
    in ExpireTimeProfileId_t expire_time_profile_id);


// SelectByDirection is a simple pure function that behaves exactly as
// the P4_16 function definition given in comments below.  It is an
// extern function to ensure that the front/mid end of the p4c
// compiler leaves occurrences of it as is, visible to target-specific
// compiler back end code, so targets have all information needed to
// optimize it as they wish.

// One example of its use is in table key expressions, for tables
// where one wishes to swap IP source/destination addresses for
// packets processed in the different directions.

/*
T SelectByDirection<T>(
    in bool is_network_port,
    in T from_net_value,
    in T from_host_value)
{
    if (is_network_port) {
        return from_net_value;
    } else {
        return from_host_value;
    }
}
*/

// A typical call would look like this example:
//
// SelectByDirection(is_net_port(istd.input_port), hdr.ipv4.src_addr,
//                               hdr.ipv4.dst_addr)

@pure
extern T SelectByDirection<T>(
    in bool is_network_port,
    in T from_net_value,
    in T from_host_value);




// BEGIN:Programmable_blocks
parser MainParserT<MH, MM>(
    packet_in pkt,
    out   MH main_hdr,
    inout MM main_user_meta,
    in    pna_main_parser_input_metadata_t istd);

control MainControlT<MH, MM>(
    inout MH main_hdr,
    inout MM main_user_meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd);

control MainDeparserT<MH, MM>(
    packet_out pkt,
    in    MH main_hdr,
    in    MM main_user_meta,
    in    pna_main_output_metadata_t ostd);

package PNA_NIC<MH, MM>(
    MainParserT<MH, MM> main_parser,
    MainControlT<MH, MM> main_control,
    MainDeparserT<MH, MM> main_deparser);
// END:Programmable_blocks

#endif   // __PNA_P4__
