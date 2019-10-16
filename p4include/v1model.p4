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

/* P4-16 declaration of the P4 v1.0 switch model */

/* Note 1: More details about the definition of v1model architecture
 * can be found at the location below.
 *
 * https://github.com/p4lang/behavioral-model/blob/master/docs/simple_switch.md
 *
 * Note 2: There are ongoing discussions among P4 working group
 * members in 2019-Apr regarding exactly how resubmit, recirculate,
 * and clone3 operations can be called anywhere in their respective
 * controls, but the values of the fields to be preserved is the value
 * they have when that control is finished executing.  That is how
 * these operations behave in P4_14, but this requires some care in
 * making this happen in P4_16.
 *
 * Note 3: There are at least some P4_14 implementations where
 * invoking a generate_digest operation on a field_list will create a
 * message to the control plane that contains the values of those
 * fields when the ingress control is finished executing, which can be
 * different than the values those fields have at the time the
 * generate_digest operation is invoked in the program, if those field
 * values are changed later in the execution of the P4_14 ingress
 * control.
 *
 * The P4_16 plus v1model implementation should always create digest
 * messages that contain the values of the specified fields at the
 * time that the digest extern function is called.  Thus if a P4_14
 * program expecting the behavior described above is compiled using
 * p4c, it may behave differently.
 */

#ifndef _V1_MODEL_P4_
#define _V1_MODEL_P4_

#include "core.p4"

match_kind {
    range,
    // Used for implementing dynamic_action_selection
    selector
}

// Are these correct?
@metadata @name("standard_metadata")
struct standard_metadata_t {
    bit<9>  ingress_port;
    bit<9>  egress_spec;
    bit<9>  egress_port;
    bit<32> instance_type;
    bit<32> packet_length;
    //
    // @alias is used to generate the field_alias section of the BMV2 JSON.
    // Field alias creates a mapping from the metadata name in P4 program to
    // the behavioral model's internal metadata name. Here we use it to
    // expose all metadata supported by simple switch to the user through
    // standard_metadata_t.
    //
    // flattening fields that exist in bmv2-ss
    // queueing metadata
    @alias("queueing_metadata.enq_timestamp")
    bit<32> enq_timestamp;
    @alias("queueing_metadata.enq_qdepth")
    bit<19> enq_qdepth;
    @alias("queueing_metadata.deq_timedelta")
    bit<32> deq_timedelta;
    /// queue depth at the packet dequeue time.
    @alias("queueing_metadata.deq_qdepth")
    bit<19> deq_qdepth;

    // intrinsic metadata
    @alias("intrinsic_metadata.ingress_global_timestamp")
    bit<48> ingress_global_timestamp;
    @alias("intrinsic_metadata.egress_global_timestamp")
    bit<48> egress_global_timestamp;
    /// multicast group id (key for the mcast replication table)
    @alias("intrinsic_metadata.mcast_grp")
    bit<16> mcast_grp;
    /// Replication ID for multicast
    @alias("intrinsic_metadata.egress_rid")
    bit<16> egress_rid;
    /// Indicates that a verify_checksum() method has failed.
    /// 1 if a checksum error was found, otherwise 0.
    bit<1>  checksum_error;
    /// Error produced by parsing
    error parser_error;
    /// set packet priority
    @alias("intrinsic_metadata.priority")
    bit<3> priority;
}

enum CounterType {
    packets,
    bytes,
    packets_and_bytes
}

enum MeterType {
    packets,
    bytes
}

extern counter {
    /***
     * A counter object is created by calling its constructor.  This
     * creates an array of counter states, with the number of counter
     * states specified by the size parameter.  The array indices are
     * in the range [0, size-1].
     *
     * You must provide a choice of whether to maintain only a packet
     * count (CounterType.packets), only a byte count
     * (CounterType.bytes), or both (CounterType.packets_and_bytes).
     */
    counter(bit<32> size, CounterType type);
    /***
     * count() causes the counter state with the specified index to be
     * read, modified, and written back, atomically relative to the
     * processing of other packets, updating the packet count, byte
     * count, or both, depending upon the CounterType of the counter
     * instance used when it was constructed.
     *
     * @param index The index of the counter state in the array to be
     *              updated, normally a value in the range [0,
     *              size-1].  If index >= size, no counter state will be
     *              updated.
     */
    void count(in bit<32> index);
}

extern direct_counter {
    /***
     * A direct_counter object is created by calling its constructor.
     * You must provide a choice of whether to maintain only a packet
     * count (CounterType.packets), only a byte count
     * (CounterType.bytes), or both (CounterType.packets_and_bytes).
     * After constructing the object, you can associate it with at
     * most one table, by adding the following table property to the
     * definition of that table:
     *
     *     counters = <object_name>;
     */
    direct_counter(CounterType type);
    /***
     * The count() method is actually unnecessary in the v1model
     * architecture.  This is because after a direct_counter object
     * has been associated with a table as described in the
     * documentation for the direct_counter constructor, every time
     * the table is applied and a table entry is matched, the counter
     * state associated with the matching entry is read, modified, and
     * written back, atomically relative to the processing of other
     * packets, regardless of whether the count() method is called in
     * the body of that action.
     */
    void count();
}

#define V1MODEL_METER_COLOR_GREEN  0
#define V1MODEL_METER_COLOR_YELLOW 1
#define V1MODEL_METER_COLOR_RED    2

extern meter {
    /***
     * A meter object is created by calling its constructor.  This
     * creates an array of meter states, with the number of meter
     * states specified by the size parameter.  The array indices are
     * in the range [0, size-1].  For example, if in your system you
     * have 128 different "flows" numbered from 0 up to 127, and you
     * want to meter each of those flows independently of each other,
     * you could do so by creating a meter object with size=128.
     *
     * You must provide a choice of whether to meter based on the
     * number of packets, regardless of their size
     * (MeterType.packets), or based upon the number of bytes the
     * packets contain (MeterType.bytes).
     */
    meter(bit<32> size, MeterType type);
    /***
     * execute_meter() causes the meter state with the specified index
     * to be read, modified, and written back, atomically relative to
     * the processing of other packets, and an integer encoding of one
     * of the colors green, yellow, or red to be written to the result
     * out parameter.
     *
     * @param index The index of the meter state in the array to be
     *              updated, normally a value in the range [0,
     *              size-1].  If index >= size, no meter state will be
     *              updated.
     * @param result Type T must be bit<W> with W >= 2.  When index is
     *              in range, the value of result will be assigned 0
     *              for color GREEN, 1 for color YELLOW, and 2 for
     *              color RED (see RFC 2697 and RFC 2698 for the
     *              meaning of these colors).  When index is out of
     *              range, the final value of result is not specified,
     *              and should be ignored by the caller.
     */
    void execute_meter<T>(in bit<32> index, out T result);
}

extern direct_meter<T> {
    /***
     * A direct_meter object is created by calling its constructor.
     * You must provide a choice of whether to meter based on the
     * number of packets, regardless of their size
     * (MeterType.packets), or based upon the number of bytes the
     * packets contain (MeterType.bytes).  After constructing the
     * object, you can associate it with at most one table, by adding
     * the following table property to the definition of that table:
     *
     *     meters = <object_name>;
     */
    direct_meter(MeterType type);
    /***
     * After a direct_meter object has been associated with a table as
     * described in the documentation for the direct_meter
     * constructor, every time the table is applied and a table entry
     * is matched, the meter state associated with the matching entry
     * is read, modified, and written back, atomically relative to the
     * processing of other packets, regardless of whether the read()
     * method is called in the body of that action.
     *
     * read() may only be called within an action executed as a result
     * of matching a table entry, of a table that has a direct_meter
     * associated with it.  Calling read() causes an integer encoding
     * of one of the colors green, yellow, or red to be written to the
     * result out parameter.
     *
     * @param result Type T must be bit<W> with W >= 2.  The value of
     *              result will be assigned 0 for color GREEN, 1 for
     *              color YELLOW, and 2 for color RED (see RFC 2697
     *              and RFC 2698 for the meaning of these colors).
     */
    void read(out T result);
}

extern register<T> {
    register(bit<32> size);
    /***
     * read() reads the state of the register array stored at the
     * specified index, and returns it as the value written to the
     * result parameter.
     *
     * @param index The index of the register array element to be
     *              read, normally a value in the range [0, size-1].
     * @param result Only types T that are bit<W> are currently
     *              supported.  When index is in range, the value of
     *              result becomes the value read from the register
     *              array element.  When index >= size, the final
     *              value of result is not specified, and should be
     *              ignored by the caller.
     */
    void read(out T result, in bit<32> index);
    /***
     * write() writes the state of the register array at the specified
     * index, with the value provided by the value parameter.
     *
     * If you wish to perform a read() followed later by a write() to
     * the same register array element, and you wish the
     * read-modify-write sequence to be atomic relative to other
     * processed packets, then there may be parallel implementations
     * of the v1model architecture for which you must execute them in
     * a P4_16 block annotated with an @atomic annotation.  See the
     * P4_16 language specification description of the @atomic
     * annotation for more details.
     *
     * @param index The index of the register array element to be
     *              written, normally a value in the range [0,
     *              size-1].  If index >= size, no register state will
     *              be updated.
     * @param value Only types T that are bit<W> are currently
     *              supported.  When index is in range, this
     *              parameter's value is written into the register
     *              array element specified by index.
     */
    void write(in bit<32> index, in T value);
}

// used as table implementation attribute
extern action_profile {
    action_profile(bit<32> size);
}

/***
 * Generate a random number in the range lo..hi, inclusive, and write
 * it to the result parameter.  The value written to result is not
 * specified if lo > hi.
 *
 * @param T          Must be a type bit<W>
 */
extern void random<T>(out T result, in T lo, in T hi);

/***
 * Calling digest causes a message containing the values specified in
 * the data parameter to be sent to the control plane software.  It is
 * similar to sending a clone of the packet to the control plane
 * software, except that it can be more efficient because the messages
 * are typically smaller than packets, and many such small digest
 * messages are typically coalesced together into a larger "batch"
 * which the control plane software processes all at once.
 *
 * The value of the fields that are sent in the message to the control
 * plane is the value they have at the time the digest call occurs,
 * even if those field values are changed by later ingress control
 * code.  See Note 3.
 *
 * Calling digest is only supported in the ingress control.  There is
 * no way to undo its effects once it has been called.
 *
 * If the type T is a named struct, the name is used to generate the
 * control plane API.
 *
 * The BMv2 implementation of the v1model architecture ignores the
 * value of the receiver parameter.
 */
extern void digest<T>(in bit<32> receiver, in T data);

enum HashAlgorithm {
    crc32,
    crc32_custom,
    crc16,
    crc16_custom,
    random,
    identity,
    csum16,
    xor16
}

@deprecated("Please use mark_to_drop(standard_metadata) instead.")
extern void mark_to_drop();

/***
 * mark_to_drop(standard_metadata) is a primitive action that modifies
 * standard_metadata.egress_spec to an implementation-specific special
 * value that in some cases causes the packet to be dropped at the end
 * of ingress or egress processing.  It also assigns 0 to
 * standard_metadata.mcast_grp.  Either of those metadata fields may
 * be changed by executing later P4 code, after calling
 * mark_to_drop(), and this can change the resulting behavior of the
 * packet to do something other than drop.
 *
 * See
 * https://github.com/p4lang/behavioral-model/blob/master/docs/simple_switch.md
 * -- in particular the section "Pseudocode for what happens at the
 * end of ingress and egress processing" -- for the relative priority
 * of the different possible things that can happen to a packet when
 * ingress and egress processing are complete.
 */
extern void mark_to_drop(inout standard_metadata_t standard_metadata);

/***
 * Calculate a hash function of the value specified by the data
 * parameter.  The value written to the out parameter named result
 * will always be in the range [base, base+max-1] inclusive, if max >=
 * 1.  If max=0, the value written to result will always be base.
 *
 * Note that the types of all of the parameters may be the same as, or
 * different from, each other, and thus their bit widths are allowed
 * to be different.
 *
 * @param O          Must be a type bit<W>
 * @param D          Must be a tuple type where all the fields are bit-fields (type bit<W> or int<W>) or varbits.
 * @param T          Must be a type bit<W>
 * @param M          Must be a type bit<W>
 */
extern void hash<O, T, D, M>(out O result, in HashAlgorithm algo, in T base, in D data, in M max);

extern action_selector {
    action_selector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
}

enum CloneType {
    I2E,
    E2E
}

@deprecated("Please use verify_checksum/update_checksum instead.")
extern Checksum16 {
    Checksum16();
    bit<16> get<D>(in D data);
}

/***
 * Verifies the checksum of the supplied data.  If this method detects
 * that a checksum of the data is not correct, then the value of the
 * standard_metadata checksum_error field will be equal to 1 when the
 * packet begins ingress processing.
 *
 * Calling verify_checksum is only supported in the VerifyChecksum
 * control.
 *
 * @param T          Must be a tuple type where all the tuple elements
 *                   are of type bit<W>, int<W>, or varbit<W>.  The
 *                   total length of the fields must be a multiple of
 *                   the output size.
 * @param O          Checksum type; must be bit<X> type.
 * @param condition  If 'false' the verification always succeeds.
 * @param data       Data whose checksum is verified.
 * @param checksum   Expected checksum of the data; note that it must
 *                   be a left-value.
 * @param algo       Algorithm to use for checksum (not all algorithms
 *                   may be supported).  Must be a compile-time
 *                   constant.
 */
extern void verify_checksum<T, O>(in bool condition, in T data, in O checksum, HashAlgorithm algo);

/***
 * Computes the checksum of the supplied data and writes it to the
 * checksum parameter.
 *
 * Calling update_checksum is only supported in the ComputeChecksum
 * control.
 *
 * @param T          Must be a tuple type where all the tuple elements
 *                   are of type bit<W>, int<W>, or varbit<W>.  The
 *                   total length of the fields must be a multiple of
 *                   the output size.
 * @param O          Output type; must be bit<X> type.
 * @param condition  If 'false' the checksum parameter is not changed
 * @param data       Data whose checksum is computed.
 * @param checksum   Checksum of the data.
 * @param algo       Algorithm to use for checksum (not all algorithms
 *                   may be supported).  Must be a compile-time
 *                   constant.
 */
extern void update_checksum<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);

/***
 * verify_checksum_with_payload is identical in all ways to
 * verify_checksum, except that it includes the payload of the packet
 * in the checksum calculation.  The payload is defined as "all bytes
 * of the packet which were not parsed by the parser".
 *
 * Calling verify_checksum_with_payload is only supported in the
 * VerifyChecksum control.
 */
extern void verify_checksum_with_payload<T, O>(in bool condition, in T data, in O checksum, HashAlgorithm algo);

/**
 * update_checksum_with_payload is identical in all ways to
 * update_checksum, except that it includes the payload of the packet
 * in the checksum calculation.  The payload is defined as "all bytes
 * of the packet which were not parsed by the parser".
 *
 * Calling update_checksum_with_payload is only supported in the
 * ComputeChecksum control.
 */
extern void update_checksum_with_payload<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);

/***
 * Calling resubmit during execution of the ingress control will,
 * under certain documented conditions, cause the packet to be
 * resubmitted, i.e. it will begin processing again with the parser,
 * with the contents of the packet exactly as they were when it last
 * began parsing.  The only difference is in the value of the
 * standard_metadata instance_type field, and any user-defined
 * metadata fields that the resubmit operation causes to be
 * preserved.
 *
 * The value of the user-defined metadata fields that are preserved in
 * resubmitted packets is the value they have at the end of ingress
 * processing, not their values at the time the resubmit call is made.
 * See Note 2.
 *
 * Calling resubmit is only supported in the ingress control.  There
 * is no way to undo its effects once it has been called.  If resubmit
 * is called multiple times during a single execution of the ingress
 * control, only one packet is resubmitted, and only the data from the
 * last such call is preserved.  See the v1model architecture
 * documentation (Note 1) for more details.
 */
extern void resubmit<T>(in T data);

/***
 * Calling recirculate during execution of the egress control will,
 * under certain documented conditions, cause the packet to be
 * recirculated, i.e. it will begin processing again with the parser,
 * with the contents of the packet as they are created by the
 * deparser.  Recirculated packets can be distinguished from new
 * packets in ingress processing by the value of the standard_metadata
 * instance_type field.  The caller may request that some user-defined
 * metadata fields be preserved with the recirculated packet.
 *
 * The value of the user-defined metadata fields that are preserved in
 * recirculated packets is the value they have at the end of egress
 * processing, not their values at the time the recirculate call is
 * made.  See Note 2.
 *
 * Calling recirculate is only supported in the egress control.  There
 * is no way to undo its effects once it has been called.  If
 * recirculate is called multiple times during a single execution of
 * the egress control, only one packet is recirculated, and only the
 * data from the last such call is preserved.  See the v1model
 * architecture documentation (Note 1) for more details.
 */
extern void recirculate<T>(in T data);

/***
 * clone is in most ways identical to the clone3 operation, with the
 * only difference being that it never preserves any user-defined
 * metadata fields with the cloned packet.  It is equivalent to
 * calling clone3 with the same type and session parameter values,
 * with empty data.
 */
extern void clone(in CloneType type, in bit<32> session);

/***
 * Calling clone3 during execution of the ingress or egress control
 * will cause the packet to be cloned, sometimes also called
 * mirroring, i.e. zero or more copies of the packet are made, and
 * each will later begin egress processing as an independent packet
 * from the original packet.  The original packet continues with its
 * normal next steps independent of the clone(s).
 *
 * The session parameter is an integer identifying a clone session id
 * (sometimes called a mirror session id).  The control plane software
 * must configure each session you wish to use, or else no clones will
 * be made using that session.  Typically this will involve the
 * control plane software specifying one output port to which the
 * cloned packet should be sent, or a list of (port, egress_rid) pairs
 * to which a separate clone should be created for each, similar to
 * multicast packets.
 *
 * Cloned packets can be distinguished from others by the value of the
 * standard_metadata instance_type field.
 *
 * The caller may request that some user-defined metadata field values
 * from the original packet should be preserved with the cloned
 * packet(s).  The value of the user-defined metadata fields that are
 * preserved with cloned packets is the value they have at the end of
 * ingress or egress processing, not their values at the time the
 * clone3 call is made.  See Note 2.
 *
 * If clone3 is called during ingress processing, the first parameter
 * must be CloneType.I2E.  If clone3 is called during egress
 * processing, the first parameter must be CloneType.E2E.
 *
 * There is no way to undo its effects once it has been called.  If
 * there are multiple calls to clone3 and/or clone during a single
 * execution of the same ingress (or egress) control, only the last
 * clone session and data are used.  See the v1model architecture
 * documentation (Note 1) for more details.
 */
extern void clone3<T>(in CloneType type, in bit<32> session, in T data);

extern void truncate(in bit<32> length);

/***
 * Calling assert when the argument is true has no effect, except any
 * effect that might occur due to evaluation of the argument (but see
 * below).  If the argument is false, the precise behavior is
 * target-specific, but the intent is to record or log which assert
 * statement failed, and optionally other information about the
 * failure.
 *
 * For example, on the simple_switch target, executing an assert
 * statement with a false argument causes a log message with the file
 * name and line number of the assert statement to be printed, and
 * then the simple_switch process exits.
 *
 * If you provide the --ndebug command line option to p4c when
 * compiling, the compiled program behaves as if all assert statements
 * were not present in the source code.
 *
 * We strongly recommend that you avoid using expressions as an
 * argument to an assert call that can have side effects, e.g. an
 * extern method or function call that has side effects.  p4c will
 * allow you to do this with no warning given.  We recommend this
 * because, if you follow this advice, your program will behave the
 * same way when assert statements are removed.
 */
extern void assert(in bool check);

/***
 * For the purposes of compiling and executing P4 programs on a target
 * device, assert and assume are identical, including the use of the
 * --ndebug p4c option to elide them.  See documentation for assert.
 *
 * The reason that assume exists as a separate function from assert is
 * because they are expected to be used differently by formal
 * verification tools.  For some formal tools, the goal is to try to
 * find example packets and sets of installed table entries that cause
 * an assert statement condition to be false.
 *
 * Suppose you run such a tool on your program, and the example packet
 * given is an MPLS packet, i.e. hdr.ethernet.etherType == 0x8847.
 * You look at the example, and indeed it does cause an assert
 * condition to be false.  However, your plan is to deploy your P4
 * program in a network in places where no MPLS packets can occur.
 * You could add extra conditions to your P4 program to handle the
 * processing of such a packet cleanly, without assertions failing,
 * but you would prefer to tell the tool "such example packets are not
 * applicable in my scenario -- never show them to me".  By adding a
 * statement:
 *
 *     assume(hdr.ethernet.etherType != 0x8847);
 *
 * at an appropriate place in your program, the formal tool should
 * never show you such examples -- only ones that make all such assume
 * conditions true.
 *
 * The reason that assume statements behave the same as assert
 * statements when compiled to a target device is that if the
 * condition ever evaluates to false when operating in a network, it
 * is likely that your assumption was wrong, and should be reexamined.
 */
extern void assume(in bool check);

/*
 * Log user defined messages
 * Example: log_msg("User defined message");
 * or log_msg("Value1 = {}, Value2 = {}",{value1, value2});
 */
extern void log_msg(string msg);
extern void log_msg<T>(string msg, in T data);

// The name 'standard_metadata' is reserved

/*
 * Architecture.
 *
 * M must be a struct.
 *
 * H must be a struct where every one if its members is of type
 * header, header stack, or header_union.
 */

parser Parser<H, M>(packet_in b,
                    out H parsedHdr,
                    inout M meta,
                    inout standard_metadata_t standard_metadata);

/*
 * The only legal statements in the body of the VerifyChecksum control
 * are: block statements, calls to the verify_checksum and
 * verify_checksum_with_payload methods, and return statements.
 */
control VerifyChecksum<H, M>(inout H hdr,
                             inout M meta);
@pipeline
control Ingress<H, M>(inout H hdr,
                      inout M meta,
                      inout standard_metadata_t standard_metadata);
@pipeline
control Egress<H, M>(inout H hdr,
                     inout M meta,
                     inout standard_metadata_t standard_metadata);

/*
 * The only legal statements in the body of the ComputeChecksum
 * control are: block statements, calls to the update_checksum and
 * update_checksum_with_payload methods, and return statements.
 */
control ComputeChecksum<H, M>(inout H hdr,
                              inout M meta);

/*
 * The only legal statements in the body of the Deparser control are:
 * calls to the packet_out.emit() method.
 */
@deparser
control Deparser<H>(packet_out b, in H hdr);

package V1Switch<H, M>(Parser<H, M> p,
                       VerifyChecksum<H, M> vr,
                       Ingress<H, M> ig,
                       Egress<H, M> eg,
                       ComputeChecksum<H, M> ck,
                       Deparser<H> dep
                       );

#endif  /* _V1_MODEL_P4_ */
