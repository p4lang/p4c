/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _PORTABLE_SWITCH_ARCHITECTURE_P4_
#define _PORTABLE_SWITCH_ARCHITECTURE_P4_

/**
 *   P4-16 declaration of the Portable Switch Architecture
 */

/**
 * These types need to be defined before including the architecture file
 * and the macro protecting them should be defined.
 */
#ifndef PSA_CORE_TYPES
typedef bit<unspecified> SessionId_t;
typedef bit<unspecified> PortId_t;
typedef bit<unspecified> MulticastGroup_t;
typedef bit<unspecified> PacketLength_t;
typedef bit<unspecified> EgressInstance_t;
typedef bit<unspecified> InstanceType_t;
typedef bit<unspecified> ParserStatus_t;
typedef bit<unspecified> ParserErrorLocation_t;
typedef bit<unspecified> entry_key;           /// for DirectMeters
const   InstanceType_t   INSTANCE_NORMAL = unspecified;
const   PortId_t         PORT_CPU = unspecified;
#endif

#include "core.p4"

/* --------------------------------------------------------------------- *
/**
 * PSA supported metadata types
 */
struct psa_parser_input_metadata_t {
  PortId_t                 ingress_port;
  InstanceType_t           instance_type;
}

struct psa_ingress_input_metadata_t {
  PortId_t                 ingress_port;
  InstanceType_t           instance_type;  /// Clone or Normal
  /// set by the runtime in the parser, these are not under programmer control
  ParserStatus_t           parser_status;
  ParserErrorLocation_t    parser_error_location;
}

struct psa_ingress_output_metadata_t {
  PortId_t                 egress_port;
}

struct psa_egress_input_metadata_t {
  PortId_t                 egress_port;
  InstanceType_t           instance_type;  /// Clone or Normal
  EgressInstance_t         instance;       /// instance coming from PRE
}

/* --------------------------------------------------------------------- */
/**
 * Additional supported match_kind types
 */
match_kind {
    range,   /// Used to represent min..max intervals
    selector /// Used for implementing dynamic_action_selection
}

/* --------------------------------------------------------------------- */
/**
 *  Cloning methods
 */
enum CloneMethod_t {
  /// Clone method         Packet source             Insertion point
  Ingress2Ingress,  /// original ingress,            Ingress parser
  Ingres2Egress,    /// post parse original ingres,  Buffering queue
  Egress2Ingress,   /// post deparse in egress,      Ingress parser
  Egress2Egress     /// inout to deparser in egress, Buffering queue
}

/* --------------------------------------------------------------------- */
/*
 * PSA Externs
 */

/**
 * The PacketReplicationEngine extern represents the non-programmable
 * part of the PSA pipeline.
 *
 * Even though the PRE can not be programmed using P4, it can be
 * configured both directly using control plane APIs and by setting
 * intrinsic metadata. In this specification we opt to define the
 * operations available in the PRE as method invocations. A target
 * backend is responsible for mapping the PRE extern APIs to the
 * appropriate mechanisms for performing these operations in the
 * hardware.
 *
 * The PRE is instantiated by the architecture and a P4 program can
 * use it directly. It is an error to instantiate the PRE multiple
 * times.  The PRE is made available to the Ingress and Egress
 * programmable blocks using the same mechanism as packet_in.
 *
 * Note: some of these operations may not be implemented as primitive
 * operations on a certain target. However, All of the operations can
 * be implemented as a combination of other operations. Applications
 * that rely on non-primitive operations may incur significant
 * performance penalties, however, they should be functionally
 * correct.
 *
 * Alternatively, we can consider a design in which we split the PRE
 * (and the associated functionality) between an ingress PRE extern
 * and a egress BQE (Buffering and Queueing Engine) extern. That
 * decision will be finalized based on application needs.
 *
 * Semantics of behavior for multiple calls to PRE APIs
 *
 * - multiple calls to send_to_port -- the last call in the ingress
 *   pipeline sets the output port.
 *
 * - multiple calls to multicast -- the last in the ingress pipeline
 *   sets the multicast group
 *
 * - interleaving send_to_port and multicast -- the semantics of multicast
 *   is defined as below (https://github.com/p4lang/tutorials/issues/22):
 *     if (multicast_group != 0)
 *        multicast_to_group(multicast_group);
 *     else
 *        send_to_port(multicast_port);
 * From this, it follows that if there is a call that sets the
 * multicast_group, the packet will be multicast to the group that
 * last set the multicast group. Otherwise, the packet will be sent to
 * the port set by either send_to_port or multicast invocation.
 *
 * - any call to drop in the pipeline will cause the packet to drop
 *
 * - how do we drop clone packets?
 * ...
 *
 * \TODO: finalize the semantics of calling multiple of the PRE APIs
 *
 */
extern PacketReplicationEngine {

  // PacketReplicationEngine(); /// No constructor. PRE is instantiated
                                /// by the architecture.

  /// Unicast operation: sends packet to port
  ///
  /// Targets may implement this operation by setting the appropriate
  /// intrinsic metadata or through some other mechanism of
  /// configuring the PRE.
  ///
  /// @param port The output port. If the port is PORT_CPU the packet
  ///             will be sent to CPU
  void send_to_port (in PortId_t port);

  /// Multicast operation: sends packet to a multicast group or a port
  ///
  /// Targets may implement this operation by setting the appropriate
  /// intrinsic metadata or through some other mechanism of
  /// configuring the PRE.
  ///
  /// @param multicast_group Multicast group id. The control plane
  ///        must program the multicast groups through a separate
  ///        mechanism.
  /// @param port The output port.
  void multicast (in MulticastGroup_t multicast_group,
                  in PortId_t multicast_port);

  /// Drop operation: do not forward the packet
  ///
  /// The PSA implements drop as an operation in the PRE. While the
  /// drop operation can be invoked anywhere in the ingress pipeline,
  /// the semantics supported by the PSA is that the drop will be at
  /// the end of the pipeline (ingress or egress).
  void drop      ();

  /// Resubmit operation: send a packet to the ingress port with
  ///                     additional data appended
  ///
  /// This operation is intended for recursive packet processing.
  ///
  /// @param data A header definition that can be added to the set of
  ///             packet headers.
  /// @param port The input port at which the packet will be resubmitted.
  void resubmit<T>(in T data, in PortId_t port);

  /// Recirculate operation: send a post deparse packet to the ingress
  ///                        port with additional data appended
  ///
  /// This operation is intended for recursive packet processing.
  ///
  /// @param data A header definition that can be added to the set of
  ///             packet headers.
  /// @param port The input port at which the packet will be resubmitted.
  void recirculate<T>(in T data, in PortId_t port);

  /// Truncate operation
  ///
  /// Truncate the outgoing packet to the specified length
  ///
  /// @param length packet length
  void truncate(in bit<32> length);

  /*
  @ControlPlaneAPI
  {
  // TBD
  }
  */
}

extern Clone {
  /// create a copy of the packet to the specified mirror session.
  ///
  /// The PSA specifies four types of cloning, with the packet sourced
  /// from different points in the pipeline and sent back to ingress
  /// or to the buffering queue in the egress (@see CloneMethod_t).
  ///
  /// @param clone_method  The type of cloning.
  /// @param session_id    Port mirror session id.
  ///
  void clone (in CloneMethod_t clone_method, in SessionId_t session_id);

  /// create a copy of the packet with additional data to the specified mirror session.
  ///
  /// The PSA specifies four types of cloning, with the packet sourced
  /// from different points in the pipeline and sent back to ingress
  /// or to the buffering queue in the egress (@see CloneMethod_t).
  ///
  /// @param clone_method  The type of cloning.
  /// @param session_id    Port mirror session id.
  /// @param data additional header data attached to the packet
  void clone<T> (in CloneMethod_t clone_method, in SessionId_t session_id, in T data);

  /// emit additional data T passed in from clone<T> in deparser.
  ///
  /// This method can only be called in deparser, if there is no additional
  /// data to emit, this method is a nop.
  void emit();
}

/* --------------------------------------------------------------------- */
/*
 * Hashes
 */

/// Supported hash algorithms
enum HashAlgorithm {
  crc32,
  crc32_custom,
  crc16,
  crc16_custom,
  random,
  identity
}

/// Hash function
///
/// @param Algo The algorithm to use for computation (@see HashAlgorithm).
/// @param O    The type of the return value of the hash.
///
/// Example usage:
/// parser P() {
///   Hash<crc16, bit<56>> h();
///   bit<56> hash_value = h.getHash(16, buffer, 100);
///   ...
/// }
extern Hash<Algo, O> {
  /// Constructor
  Hash();

  /// compute the hash for data
  O getHash<T, D, M>(in T base, in D data, in M max);

  /*
  @ControlPlaneAPI
  { }
  */
}

/* --------------------------------------------------------------------- */
/// Checksum computation
///
/// @param W    The width of the checksum
extern Checksum<W> {
  Checksum(HashAlgorithm hash);          /// constructor
  void clear();              /// prepare unit for computation
  void update<T>(in T data); /// add data to checksum
  void remove<T>(in T data); /// remove data from existing checksum
  W    get();      	     /// get the checksum for data added since last clear

  /*
  @ControlPlaneAPI
  { }
  */
}

/* --------------------------------------------------------------------- */
/**
 * Counters
 *
 * Counters are a simple mechanism for keeping statistics about the
 * packets that trigger a table in a Match Action unit.
 *
 * Direct counters fire when the count method is ivoked in an action, and
 * have an instance for each entry in the table.
 */

/// Counter types
enum CounterType_t {
    packets,
    bytes,
    packets_and_bytes
}

/// Counter: RFC-XXXX
extern Counter<W, S> {
  Counter(S n_counters, W size_in_bits, CounterType_t counter_type);
  void count(in S index, in W increment);

  /*
  @ControlPlaneAPI
  {
    W    read<W>      (in S index);
    W    sync_read<W> (in S index);
    void set          (in S index, in W seed);
    void reset        (in S index);
    void start        (in S index);
    void stop         (in S index);
  }
  */
}

/// DirectCounter: RFC-XXXX
extern DirectCounter<W> {
  DirectCounter(entry_key key, CounterType_t counter_type);
  void count();

  /*
  @ControlPlaneAPI
  {
    W    read<W>      (in entry_key key);
    W    sync_read<W> (in entry_key key);
    void set          (in W seed);
    void reset        (in entry_key key);
    void start        (in entry_key key);
    void stop         (in entry_key key);
  }
  */
}

/* --------------------------------------------------------------------- */
/**
 * Meters: RFC 2698
 *
 * Meters are a more complex mechanism for keeping statistics about
 * the packets that trigger a table. The meters specified in the PSA
 * are 3-color meters.
 */

/// Meter types
enum MeterType_t {
    packets,
    bytes
}
/// Meter colors
enum MeterColor_t { RED, GREEN, YELLOW };

/// Meter
extern Meter<S> {
  Meter(S n_meters, MeterType_t type);
  MeterColor_t execute(in S index, in MeterColor_t color);

  /*
  @ControlPlaneAPI
  {
    reset(in MeterColor_t color);
    setParams(in S committedRate, out S committedBurstSize
              in S peakRate, in S peakBurstSize);
    getParams(out S committedRate, out S committedBurstSize
              out S peakRate, out S peakBurstSize);
  }
  */
}

/// DirectMeter
extern DirectMeter {
  DirectMeter(MeterType_t type);
  MeterColor_t execute(in MeterColor_t color);

  /*
  @ControlPlaneAPI
  {
    reset(in MeterColor_t color);
    void setParams<S>(in S committedRate, out S committedBurstSize
                      in S peakRate, in S peakBurstSize);
    void getParams<S>(out S committedRate, out S committedBurstSize
                      out S peakRate, out S peakBurstSize);
  }
  */
}

/* --------------------------------------------------------------------- */
/**
 * Registers
 *
 * Registers are stateful memories whose values can be read and
 * written in actions. Registers are similar to counters, but can be
 * used in a more general way to keep state.
 *
 * Although registers cannot be used directly in matching, they may be
 * used as the RHS of an assignment operation, allowing the current
 * value of the register to be copied into metadata and be available
 * for matching in subsquent tables.
 */
extern Register<T, S> {
  Register(S size);
  T    read  (in S index);
  void write (in S index, in T value);

  /*
  @ControlPlaneAPI
  {
    T    read<T>      (in S index);
    void set          (in S index, in T seed);
    void reset        (in S index);
  }
  */
}

/* --------------------------------------------------------------------- */
/**
 * Random
 *
 * The random extern provides a reliable, target specific number generator
 * in the min .. max range.
 */

/// The set of distributions supported by the Random extern.
enum RandomDistribution {
  PRNG,
  Binomial,
  Poisson
}

extern Random<T> {
  Random(RandomDistribution dist, T min, T max);
  T read();

  /*
  @ControlPlaneAPI
  {
    void reset();
    void setSeed(in T seed);
  }
  */
}

/* --------------------------------------------------------------------- */
/**
 *  Action profiles are used as table implementation attributes
 *
 * Action profiles implement a mechanism to populate table entries
 * with actions and action data. The only data plane operation
 * required is to instantiate this extern. When the control plane adds
 * entries (members) into the extern, they are essentially populating
 * the corresponding table entries.
 */
extern ActionProfile {
  /// Construct an action profile of 'size' entries
  ActionProfile(bit<32> size);

  /*
  @ControlPlaneAPI
  {
     entry_handle add_member    (action_ref, action_data);
     void         delete_member (entry_handle);
     entry_handle modify_member (entry_handle, action_ref, action_data);
  }
  */
}

/**
 *  Action selectors are used as table implementation attributes
 *
 * Action selectors implement another mechanism to populate table
 * entries with actions and action data. They are similar to action
 * profiles, with additional support to define groups of
 * entries. Action selectors require a hash algorithm to select
 * members in a group. The only data plane operation required is to
 * instantiate this extern. When the control plane adds entries
 * (members) into the extern, they are essentially populating the
 * corresponding table entries.
 */
extern ActionSelector {
  /// Construct an action selector of 'size' entries
  /// @param algo hash algorithm to select a member in a group
  /// @param size number of entries in the action selector
  /// @param outputWidth size of the key
  ActionSelector(HashAlgorithm algo, bit<32> size, bit<32> outputWidth);

  /*
  @ControlPlaneAPI
  {
     entry_handle add_member        (action_ref, action_data);
     void         delete_member     (entry_handle);
     entry_handle modify_member     (entry_handle, action_ref, action_data);
     group_handle create_group      ();
     void         delete_group      (group_handle);
     void         add_to_group      (group_handle, entry_handle);
     void         delete_from_group (group_handle, entry_handle);
  }
  */
}


/* --------------------------------------------------------------------- */
/// \TODO: is generating a new packet and sending it to the stream or is
/// it adding a header to the current packet and sending it to the
/// stream (copying or redirecting).
extern Digest<T> {
  Digest(PortId_t receiver); /// define a digest stream to receiver
  void emit(in T data);      /// emit data into the stream

  /*
  @ControlPlaneAPI
  {
  // TBD
  // If the type T is a named struct, the name should be used
  // to generate the control-plane API.
  }
  */
}

/* --------------------------------------------------------------------- */
/**
 * Programmable blocks
 *
 * The following declarations provide a template for the programmable
 * blocks in the PSA. The P4 programmer is responsible for
 * implementing controls that match these interfaces and instantiate
 * them in a package definition.
 */

parser Parser<H, M>(packet_in buffer, out H parsed_hdr, inout M user_meta,
                    in psa_parser_input_metadata_t istd);

control VerifyChecksum<H, M>(in H hdr, inout M user_meta);

control Ingress<H, M>(inout H hdr, inout M user_meta,
                      PacketReplicationEngine pre,
                      in  psa_ingress_input_metadata_t  istd,
                      out psa_ingress_output_metadata_t ostd);

control Egress<H, M>(inout H hdr, inout M user_meta,
                     PacketReplicationEngine pre,
                     in  psa_egress_input_metadata_t  istd);

control ComputeChecksum<H, M>(inout H hdr, inout M user_meta);

@deparser
control Deparser<H>(packet_out buffer, in H hdr);


package PSA_Switch<H, MP, MV, MI, ME, MC>(Parser<H, MP> p,
                                          VerifyChecksum<H, MV> vr,
                                          Ingress<H, MI> ig,
                                          Egress<H, ME> eg,
                                          ComputeChecksum<H, MC> ck,
                                          Deparser<H> dep);

#endif  /* _PORTABLE_SWITCH_ARCHITECTURE_P4_ */
