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
typedef bit<32> VportIdUint_t;
typedef bit<32> MulticastGroupUint_t;
typedef bit<16> CloneSessionIdUint_t;
typedef bit<8>  ClassOfServiceUint_t;
typedef bit<16> PacketLengthUint_t;
typedef bit<16> MulticastInstanceUint_t;
typedef bit<64> TimestampUint_t;

typedef bit<32> SecurityAssocIdUint_t;

/* Note: clone_spec in BMv2 simple_switch v1model is 32 bits wide, but
 * it is used such that 16 of its bits contain a clone/mirror session
 * id, and 16 bits contain the numeric id of a field_list.  Only the
 * 16 bits of clone/mirror session id are comparable to the type
 * CloneSessionIdUint_t here.  See occurrences of clone_spec in this
 * file for details:
 * https://github.com/p4lang/behavioral-model/blob/master/targets/simple_switch/simple_switch.cpp
 */

@p4runtime_translation("p4.org/pna/v1/PortId_t", 32)
type PortIdUint_t         PortId_t;
@p4runtime_translation("p4.org/pna/v1/VportId_t", 32)
type VportIdUint_t        VportId_t;
@p4runtime_translation("p4.org/pna/v1/MulticastGroup_t", 32)
type MulticastGroupUint_t MulticastGroup_t;
@p4runtime_translation("p4.org/pna/v1/CloneSessionId_t", 16)
type CloneSessionIdUint_t CloneSessionId_t;
@p4runtime_translation("p4.org/pna/v1/ClassOfService_t", 8)
type ClassOfServiceUint_t ClassOfService_t;
@p4runtime_translation("p4.org/pna/v1/PacketLength_t", 16)
type PacketLengthUint_t   PacketLength_t;
@p4runtime_translation("p4.org/pna/v1/MulticastInstance_t", 16)
type MulticastInstanceUint_t MulticastInstance_t;
@p4runtime_translation("p4.org/pna/v1/Timestamp_t", 64)
type TimestampUint_t      Timestamp_t;

@p4runtime_translation("p4.org/pna/v1/SecurityAssocId_t", 64)
type SecurityAssocIdUint_t      SecurityAssocId_t;

typedef error   ParserError_t;

const PortId_t PNA_PORT_CPU = (PortId_t) 0xfffffffd;

const CloneSessionId_t PNA_CLONE_SESSION_TO_CPU = (CloneSessionId_t) 0;

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
typedef bit<unspecified> VportIdUint_t;
typedef bit<unspecified> MulticastGroupUint_t;
typedef bit<unspecified> CloneSessionIdUint_t;
typedef bit<unspecified> ClassOfServiceUint_t;
typedef bit<unspecified> PacketLengthUint_t;
typedef bit<unspecified> MulticastInstanceUint_t;
typedef bit<unspecified> TimestampUint_t;

typedef bit<unspecified> SecurityAssocIdUint_t;

@p4runtime_translation("p4.org/pna/v1/PortId_t", 32)
type PortIdUint_t         PortId_t;
@p4runtime_translation("p4.org/pna/v1/VportId_t", 32)
type VportIdUint_t         VportId_t;
@p4runtime_translation("p4.org/pna/v1/MulticastGroup_t", 32)
type MulticastGroupUint_t MulticastGroup_t;
@p4runtime_translation("p4.org/pna/v1/CloneSessionId_t", 16)
type CloneSessionIdUint_t CloneSessionId_t;
@p4runtime_translation("p4.org/pna/v1/ClassOfService_t", 8)
type ClassOfServiceUint_t ClassOfService_t;
@p4runtime_translation("p4.org/pna/v1/PacketLength_t", 16)
type PacketLengthUint_t   PacketLength_t;
@p4runtime_translation("p4.org/pna/v1/MulticastInstance_t", 16)
type MulticastInstanceUint_t MulticastInstance_t;
@p4runtime_translation("p4.org/pna/v1/Timestamp_t", 64)
type TimestampUint_t      Timestamp_t;

@p4runtime_translation("p4.org/pna/v1/SecurityAssocId_t", 64)
type SecurityAssocIdUint_t      SecurityAssocId_t;

typedef error   ParserError_t;

const PortId_t PNA_PORT_CPU = (PortId_t) unspecified;

const CloneSessionId_t PNA_CLONE_SESSION_TO_CPU = (CloneSessiontId_t) unspecified;
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
typedef bit<32> VportIdInHeaderUint_t;
typedef bit<32> MulticastGroupInHeaderUint_t;
typedef bit<16> CloneSessionIdInHeaderUint_t;
typedef bit<8>  ClassOfServiceInHeaderUint_t;
typedef bit<16> PacketLengthInHeaderUint_t;
typedef bit<16> MulticastInstanceInHeaderUint_t;
typedef bit<64> TimestampInHeaderUint_t;

typedef bit<32> SecurityAssocIdInHeaderUint_t;

@p4runtime_translation("p4.org/pna/v1/PortIdInHeader_t", 32)
type  PortIdInHeaderUint_t         PortIdInHeader_t;
@p4runtime_translation("p4.org/pna/v1/VportIdInHeader_t", 32)
type  VportIdInHeaderUint_t         VportIdInHeader_t;
@p4runtime_translation("p4.org/pna/v1/MulticastGroupInHeader_t", 32)
type  MulticastGroupInHeaderUint_t MulticastGroupInHeader_t;
@p4runtime_translation("p4.org/pna/v1/CloneSessionIdInHeader_t", 16)
type  CloneSessionIdInHeaderUint_t CloneSessionIdInHeader_t;
@p4runtime_translation("p4.org/pna/v1/ClassOfServiceInHeader_t", 8)
type  ClassOfServiceInHeaderUint_t ClassOfServiceInHeader_t;
@p4runtime_translation("p4.org/pna/v1/PacketLengthInHeader_t", 16)
type  PacketLengthInHeaderUint_t   PacketLengthInHeader_t;
@p4runtime_translation("p4.org/pna/v1/MulticastInstanceInHeader_t", 16)
type  MulticastInstanceInHeaderUint_t MulticastInstanceInHeader_t;
@p4runtime_translation("p4.org/pna/v1/TimestampInHeader_t", 64)
type  TimestampInHeaderUint_t      TimestampInHeader_t;

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
MulticastGroup_t pna_MulticastGroup_header_to_int (in MulticastGroupInHeader_t x) {
    return (MulticastGroup_t) (MulticastGroupUint_t) (MulticastGroupInHeaderUint_t) x;
}
CloneSessionId_t pna_CloneSessionId_header_to_int (in CloneSessionIdInHeader_t x) {
    return (CloneSessionId_t) (CloneSessionIdUint_t) (CloneSessionIdInHeaderUint_t) x;
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

PortIdInHeader_t pna_PortId_int_to_header (in PortId_t x) {
    return (PortIdInHeader_t) (PortIdInHeaderUint_t) (PortIdUint_t) x;
}
MulticastGroupInHeader_t pna_MulticastGroup_int_to_header (in MulticastGroup_t x) {
    return (MulticastGroupInHeader_t) (MulticastGroupInHeaderUint_t) (MulticastGroupUint_t) x;
}
CloneSessionIdInHeader_t pna_CloneSessionId_int_to_header (in CloneSessionId_t x) {
    return (CloneSessionIdInHeader_t) (CloneSessionIdInHeaderUint_t) (CloneSessionIdUint_t) x;
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

/// Supported range of values for the pna_idle_timeout table properties
enum PNA_IdleTimeout_t {
    NO_TIMEOUT,
    NOTIFY_CONTROL
};

// BEGIN:Match_kinds
match_kind {
    range,   /// Used to represent min..max intervals
    selector /// Used for dynamic action selection via the ActionSelector extern
}
// END:Match_kinds

// BEGIN:Hash_algorithms
enum PNA_HashAlgorithm_t {
  // TBD what this type's values will be for PNA
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

extern Counter<W, S> {
  Counter(bit<32> n_counters, PNA_CounterType_t type);
  void count(in S index);
}
// END:Counter_extern

// BEGIN:DirectCounter_extern
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

enum bit<1> PNA_Direction_t {
    NET_TO_HOST = 0,
    HOST_TO_NET = 1
}

// BEGIN:Metadata_types
enum PNA_PacketPath_t {
    // TBD if this type remains, whether it should be an enum or
    // several separate fields representing the same cases in a
    // different form.
    FROM_NET_PORT,
    FROM_NET_LOOPEDBACK,
    FROM_NET_RECIRCULATED,
    FROM_HOST,
    FROM_HOST_LOOPEDBACK,
    FROM_HOST_RECIRCULATED
}

struct pna_pre_parser_input_metadata_t {
    PortId_t                 input_port;
    PNA_PacketPath_t         packet_path;
}

struct pna_pre_input_metadata_t {
    PortId_t                 input_port;
    PNA_PacketPath_t         packet_path;
    ParserError_t            parser_error;
}

struct pna_pre_output_metadata_t {
    bool                     drop;             // false
}

struct pna_decrypt_input_metadata_t {
    bool                     decrypt;  // TBD: or use said==0 to mean no decrypt?

    // The following things are stored internally within the decrypt
    // block, in a table indexed by said:

    // + The decryption algorithm, e.g. AES256, etc.
    // + The decryption key
    // + Any read-modify-write state in the data plane used to
    //   implement anti-replay attack detection.

    SecurityAssocId_t        said;
    bit<16>                  decrypt_start_offset;  // in bytes?

    // TBD whether it is important to explicitly pass information to a
    // decryption extern in a way visible to a P4 program about where
    // headers were parsed and found.  An alternative is to assume
    // that the architecture saves the pre parser results somewhere,
    // in a way not visible to the P4 program.
}

struct pna_main_parser_input_metadata_t {
  // common fields initialized for all packets that are input to main
  // parser, regardless of direction.
  PNA_Direction_t          direction;
  PNA_PacketPath_t         packet_path;

  // input fields to main parser that are only initialized if
  // direction == NET_TO_HOST
  PortId_t                 input_port;

  // input fields to main parser that are only initialized if
  // direction == HOST_TO_NET
  VportId_t                input_vport;
}

struct pna_main_input_metadata_t {
  // common fields initialized for all packets that are input to main
  // parser, regardless of direction.
  PNA_Direction_t          direction;
  PNA_PacketPath_t         packet_path;
  Timestamp_t              timestamp;
  ParserError_t            parser_error;
  ClassOfService_t         class_of_service;

  // input fields to main control that are only initialized if
  // direction == NET_TO_HOST
  PortId_t                 input_port;

  // input fields to main control that are only initialized if
  // direction == HOST_TO_NET
  VportId_t                input_vport;
}

// BEGIN:Metadata_main_output
struct pna_main_output_metadata_t {
  // common fields used by the architecture to decide what to do with
  // the packet next, after the main parser, control, and deparser
  // have finished executing one pass, regardless of the direction.
  bool                     drop;             // false ?
  bool                     recirculate;      // false
  ClassOfService_t         class_of_service; // 0
  bool                     clone;            // false
  CloneSessionId_t         clone_session_id; // initial value is undefined

  // output fields from main control that are only used by PNA device
  // to decide what to do with the packet next if direction ==
  // NET_TO_HOST
  bool                     host_loopback;    // false
  VportId_t                dest_vport;       // initial value is undefined

  // output fields from main control that are only used by PNA device
  // to decide what to do with the packet next if direction ==
  // HOST_TO_NET
  bool                     net_loopback;     // false
  PortId_t                 dest_port;        // initial value is undefined
}
// END:Metadata_main_output
// END:Metadata_types

// BEGIN:Programmable_blocks
parser PreParserT<PH, PM>(
    packet_in pkt,
    out PH pre_hdr,
    out PM pre_user_meta,
    in  pna_pre_parser_input_metadata_t istd);

control PreControlT<PH, PM>(
    in    PH pre_hdr,
    inout PM pre_user_meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd);

parser MainParserT<PM, MH, MM>(
    packet_in pkt,
    in    PM pre_user_meta,
    out   MH main_hdr,
    inout MM main_user_meta,
    in    pna_main_parser_input_metadata_t istd);

control MainControlT<PM, MH, MM>(
    in    PM pre_user_meta,
    inout MH main_hdr,
    inout MM main_user_meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd);

control MainDeparserT<MH, MM>(
    packet_out pkt,
    in    MH main_hdr,
    in    MM main_user_meta,
    in    pna_main_output_metadata_t ostd);

package PNA_NIC<PH, PM, MH, MM>(
    PreParserT<PH, PM> pre_parser,
    PreControlT<PH, PM> pre_control,
    MainParserT<PM, MH, MM> main_parser,
    MainControlT<PM, MH, MM> main_control,
    MainDeparserT<MH, MM> main_deparser);
// END:Programmable_blocks

#endif   // __PNA_P4__
