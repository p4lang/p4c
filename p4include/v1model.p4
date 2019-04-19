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
    bit<32> clone_spec;
    bit<32> instance_type;
    // The drop and recirculate_port fields are not used at all by the
    // behavioral-model simple_switch software switch as of September
    // 2018, and perhaps never was.  They may be considered
    // deprecated, at least for that P4 target device.  simple_switch
    // uses the value of the egress_spec field to determine whether a
    // packet is dropped or not, and it is recommended to use the
    // P4_14 drop() primitive action, or the P4_16 + v1model
    // mark_to_drop() primitive action, to cause that field to be
    // changed so the packet will be dropped.
    bit<1>  drop;
    bit<16> recirculate_port;
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
    @alias("queueing_metadata.enq_timestamp") bit<32> enq_timestamp;
    @alias("queueing_metadata.enq_qdepth")    bit<19> enq_qdepth;
    @alias("queueing_metadata.deq_timedelta") bit<32> deq_timedelta;
    @alias("queueing_metadata.deq_qdepth")    bit<19> deq_qdepth;
    // intrinsic metadata
    @alias("intrinsic_metadata.ingress_global_timestamp") bit<48> ingress_global_timestamp;
    @alias("intrinsic_metadata.egress_global_timestamp") bit<48> egress_global_timestamp;
    @alias("intrinsic_metadata.lf_field_list") bit<32> lf_field_list;
    @alias("intrinsic_metadata.mcast_grp")     bit<16> mcast_grp;
    @alias("intrinsic_metadata.resubmit_flag") bit<32> resubmit_flag;
    @alias("intrinsic_metadata.egress_rid")    bit<16> egress_rid;
    /// Indicates that a verify_checksum() method has failed.
    // 1 if a checksum error was found, otherwise 0.
    bit<1>  checksum_error;
    @alias("intrinsic_metadata.recirculate_flag") bit<32> recirculate_flag;
    /// Error produced by parsing
    error parser_error;
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

// If the type T is a named struct, the name is used
// to generate the control-plane API.
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

/**
Verifies the checksum of the supplied data.
If this method detects that a checksum of the data is not correct it
sets the standard_metadata checksum_error bit.
@param T          Must be a tuple type where all the tuple elements are of type bit<W>, int<W>, or varbit<W>.
                  The total dynamic length of the fields is a multiple of the output size.
@param O          Checksum type; must be bit<X> type.
@param condition  If 'false' the verification always succeeds.
@param data       Data whose checksum is verified.
@param checksum   Expected checksum of the data; note that is must be a left-value.
@param algo       Algorithm to use for checksum (not all algorithms may be supported).
                  Must be a compile-time constant.
*/
extern void verify_checksum<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);
/**
Computes the checksum of the supplied data.
@param T          Must be a tuple type where all the tuple elements are of type bit<W>, int<W>, or varbit<W>.
                  The total dynamic length of the fields is a multiple of the output size.
@param O          Output type; must be bit<X> type.
@param condition  If 'false' the checksum is not changed
@param data       Data whose checksum is computed.
@param checksum   Checksum of the data.
@param algo       Algorithm to use for checksum (not all algorithms may be supported).
                  Must be a compile-time constant.
*/
extern void update_checksum<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);

/**
Verifies the checksum of the supplied data including the payload.
The payload is defined as "all bytes of the packet which were not parsed by the parser".
If this method detects that a checksum of the data is not correct it
sets the standard_metadata checksum_error bit.
@param T          Must be a tuple type where all the tuple elements are of type bit<W>, int<W>, or varbit<W>.
                  The total dynamic length of the fields is a multiple of the output size.
@param O          Checksum type; must be bit<X> type.
@param condition  If 'false' the verification always succeeds.
@param data       Data whose checksum is verified.
@param checksum   Expected checksum of the data; note that is must be a left-value.
@param algo       Algorithm to use for checksum (not all algorithms may be supported).
                  Must be a compile-time constant.
*/
extern void verify_checksum_with_payload<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);
/**
Computes the checksum of the supplied data including the payload.
The payload is defined as "all bytes of the packet which were not parsed by the parser".
@param T          Must be a tuple type where all the tuple elements are of type bit<W>, int<W>, or varbit<W>.
                  The total dynamic length of the fields is a multiple of the output size.
@param O          Output type; must be bit<X> type.
@param condition  If 'false' the checksum is not changed
@param data       Data whose checksum is computed.
@param checksum   Checksum of the data.
@param algo       Algorithm to use for checksum (not all algorithms may be supported).
                  Must be a compile-time constant.
*/
extern void update_checksum_with_payload<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);

extern void resubmit<T>(in T data);
extern void recirculate<T>(in T data);
extern void clone(in CloneType type, in bit<32> session);
extern void clone3<T>(in CloneType type, in bit<32> session, in T data);

extern void truncate(in bit<32> length);

// The name 'standard_metadata' is reserved

// Architecture.
// M should be a struct of structs
// H should be a struct of headers, stacks or header_unions

parser Parser<H, M>(packet_in b,
                    out H parsedHdr,
                    inout M meta,
                    inout standard_metadata_t standard_metadata);

/* The only legal statements in the implementation of the
VerifyChecksum control are: block statements, calls to the
verify_checksum and verify_checksum_with_payload methods,
and return statements. */
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

/* The only legal statements in the implementation of the
ComputeChecksum control are: block statements, calls to the
update_checksum and update_checksum_with_payload methods,
and return statements. */
control ComputeChecksum<H, M>(inout H hdr,
                              inout M meta);

/* The only legal statements in a Deparser control are: calls to the
packet_out.emit() method. */
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
