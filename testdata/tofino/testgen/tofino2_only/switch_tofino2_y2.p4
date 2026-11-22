/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

error {
    NoError,
    PacketTooShort,
    NoMatch,
    StackOutOfBounds,
    HeaderTooShort,
    ParserTimeout,
    ParserInvalidArgument,
    CounterRange,
    Timeout,
    PhvOwner,
    MultiWrite,
    IbufOverflow,
    IbufUnderflow
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> variableFieldSizeInBits);
    T lookahead<T>();
    void advance(in bit<32> sizeInBits);
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

extern void verify(in bool check, in error toSignal);
@noWarn("unused") action NoAction() {
}
match_kind {
    exact,
    ternary,
    lpm
}

extern bool static_assert(bool check, string message);
extern bool static_assert(bool check);
typedef bit<9> PortId_t;
typedef bit<16> MulticastGroupId_t;
typedef bit<7> QueueId_t;
typedef bit<4> MirrorType_t;
typedef bit<8> MirrorId_t;
typedef bit<3> ResubmitType_t;
typedef bit<3> DigestType_t;
typedef bit<16> ReplicationId_t;
typedef bit<16> L1ExclusionId_t;
typedef bit<9> L2ExclusionId_t;
typedef MirrorType_t CloneId_t;
typedef error ParserError_t;
const bit<32> PORT_METADATA_SIZE = 32w192;
const bit<16> PARSER_ERROR_OK = 16w0x0;
const bit<16> PARSER_ERROR_NO_MATCH = 16w0x1;
const bit<16> PARSER_ERROR_PARTIAL_HDR = 16w0x2;
const bit<16> PARSER_ERROR_CTR_RANGE = 16w0x4;
const bit<16> PARSER_ERROR_TIMEOUT_USER = 16w0x8;
const bit<16> PARSER_ERROR_TIMEOUT_HW = 16w0x10;
const bit<16> PARSER_ERROR_SRC_EXT = 16w0x20;
const bit<16> PARSER_ERROR_PHV_OWNER = 16w0x80;
const bit<16> PARSER_ERROR_MULTIWRITE = 16w0x100;
const bit<16> PARSER_ERROR_ARAM_MBE = 16w0x400;
const bit<16> PARSER_ERROR_FCS = 16w0x800;
const bit<16> PARSER_ERROR_CSUM_MBE = 16w0x1000;
enum MeterType_t {
    PACKETS,
    BYTES
}

enum bit<8> MeterColor_t {
    GREEN = 8w0,
    YELLOW = 8w1,
    RED = 8w3
}

enum CounterType_t {
    PACKETS,
    BYTES,
    PACKETS_AND_BYTES
}

enum SelectorMode_t {
    FAIR,
    RESILIENT
}

enum HashAlgorithm_t {
    IDENTITY,
    RANDOM,
    XOR8,
    XOR16,
    XOR32,
    CRC8,
    CRC16,
    CRC32,
    CRC64,
    CUSTOM
}

match_kind {
    range,
    selector,
    dleft_hash,
    atcam_partition_index
}

@__intrinsic_metadata header ingress_intrinsic_metadata_t {
    bit<1>           resubmit_flag;
    @padding
    bit<1>           _pad1;
    bit<2>           packet_version;
    @padding
    bit<(4 - 9 % 8)> _pad2;
    PortId_t         ingress_port;
    bit<48>          ingress_mac_tstamp;
}

@__intrinsic_metadata struct ingress_intrinsic_metadata_for_tm_t {
    PortId_t           ucast_egress_port;
    bit<1>             bypass_egress;
    bit<1>             deflect_on_drop;
    bit<3>             ingress_cos;
    QueueId_t          qid;
    bit<3>             icos_for_copy_to_cpu;
    bit<1>             copy_to_cpu;
    bit<2>             packet_color;
    bit<1>             disable_ucast_cutthru;
    bit<1>             enable_mcast_cutthru;
    MulticastGroupId_t mcast_grp_a;
    MulticastGroupId_t mcast_grp_b;
    bit<13>            level1_mcast_hash;
    bit<13>            level2_mcast_hash;
    L1ExclusionId_t    level1_exclusion_id;
    L2ExclusionId_t    level2_exclusion_id;
    bit<16>            rid;
}

@__intrinsic_metadata struct ingress_intrinsic_metadata_from_parser_t {
    bit<48> global_tstamp;
    bit<32> global_ver;
    bit<16> parser_err;
}

@__intrinsic_metadata struct ingress_intrinsic_metadata_for_deparser_t {
    bit<3>         drop_ctl;
    DigestType_t   digest_type;
    ResubmitType_t resubmit_type;
    MirrorType_t   mirror_type;
    bit<1>         mirror_io_select;
    bit<13>        mirror_hash;
    bit<3>         mirror_ingress_cos;
    bit<1>         mirror_deflect_on_drop;
    bit<1>         mirror_multicast_ctrl;
    PortId_t       mirror_egress_port;
    QueueId_t      mirror_qid;
    bit<8>         mirror_coalesce_length;
    bit<32>        adv_flow_ctl;
    bit<14>        mtu_trunc_len;
    bit<1>         mtu_trunc_err_f;
    bit<1>         pktgen;
    bit<14>        pktgen_address;
    bit<10>        pktgen_length;
}

@__intrinsic_metadata @__ghost_metadata header ghost_intrinsic_metadata_t {
    bit<1>  ping_pong;
    bit<18> qlength;
    bit<11> qid;
    bit<2>  pipe_id;
}

@__intrinsic_metadata header egress_intrinsic_metadata_t {
    @padding
    bit<(8 - 9 % 8)> _pad0;
    PortId_t         egress_port;
    @padding
    bit<5>           _pad1;
    bit<19>          enq_qdepth;
    @padding
    bit<6>           _pad2;
    bit<2>           enq_congest_stat;
    bit<32>          enq_tstamp;
    @padding
    bit<5>           _pad3;
    bit<19>          deq_qdepth;
    @padding
    bit<6>           _pad4;
    bit<2>           deq_congest_stat;
    bit<8>           app_pool_congest_stat;
    bit<32>          deq_timedelta;
    bit<16>          egress_rid;
    @padding
    bit<7>           _pad5;
    bit<1>           egress_rid_first;
    @padding
    bit<(8 - 7 % 8)> _pad6;
    QueueId_t        egress_qid;
    @padding
    bit<5>           _pad7;
    bit<3>           egress_cos;
    @padding
    bit<7>           _pad8;
    bit<1>           deflection_flag;
    bit<16>          pkt_length;
    @padding
    bit<8>           _pad9;
}

@__intrinsic_metadata struct egress_intrinsic_metadata_from_parser_t {
    bit<48> global_tstamp;
    bit<32> global_ver;
    bit<16> parser_err;
}

@__intrinsic_metadata struct egress_intrinsic_metadata_for_deparser_t {
    bit<3>       drop_ctl;
    MirrorType_t mirror_type;
    bit<1>       coalesce_flush;
    bit<7>       coalesce_length;
    bit<1>       mirror_io_select;
    bit<13>      mirror_hash;
    bit<3>       mirror_ingress_cos;
    bit<1>       mirror_deflect_on_drop;
    bit<1>       mirror_multicast_ctrl;
    PortId_t     mirror_egress_port;
    QueueId_t    mirror_qid;
    bit<8>       mirror_coalesce_length;
    bit<32>      adv_flow_ctl;
    bit<14>      mtu_trunc_len;
    bit<1>       mtu_trunc_err_f;
}

@__intrinsic_metadata struct egress_intrinsic_metadata_for_output_port_t {
    bit<1> capture_tstamp_on_tx;
    bit<1> update_delay_on_tx;
}

header pktgen_timer_header_t {
    @padding
    bit<2>  _pad1;
    bit<2>  pipe_id;
    bit<4>  app_id;
    @padding
    bit<8>  _pad2;
    bit<16> batch_id;
    bit<16> packet_id;
}

header pktgen_port_down_header_t {
    @padding
    bit<2>            _pad1;
    bit<2>            pipe_id;
    bit<4>            app_id;
    @padding
    bit<(16 - 9 % 8)> _pad2;
    PortId_t          port_num;
    bit<16>           packet_id;
}

header pktgen_recirc_header_t {
    @padding
    bit<2>  _pad1;
    bit<2>  pipe_id;
    bit<4>  app_id;
    @padding
    bit<8>  _pad2;
    bit<16> batch_id;
    bit<16> packet_id;
}

header pktgen_deparser_header_t {
    @padding
    bit<2>  _pad1;
    bit<2>  pipe_id;
    bit<4>  app_id;
    @padding
    bit<8>  _pad2;
    bit<16> batch_id;
    bit<16> packet_id;
}

header pktgen_pfc_header_t {
    @padding
    bit<2>  _pad1;
    bit<2>  pipe_id;
    bit<4>  app_id;
    @padding
    bit<40> _pad2;
}

header ptp_metadata_t {
    bit<8>  udp_cksum_byte_offset;
    bit<8>  cf_byte_offset;
    bit<48> updated_cf;
}

extern Checksum {
    Checksum();
    void add<T>(in T data);
    void subtract<T>(in T data);
    bool verify();
    bit<16> get();
    void subtract_all_and_deposit<T>(out T residual);
    bit<16> update<T>(in T data, @optional in bool zeros_as_ones);
}

extern ParserCounter {
    ParserCounter();
    void set<T>(in T value);
    void set<T>(in T field, in bit<8> max, in bit<8> rotate, in bit<8> mask, in bit<8> add);
    void push<T>(in T value, @optional bool update_with_top);
    void push<T>(in T field, in bit<8> max, in bit<8> rotate, in bit<8> mask, in bit<8> add, @optional bool update_with_top);
    void pop();
    bool is_zero();
    bool is_negative();
    void increment(in bit<8> value);
    void decrement(in bit<8> value);
}

extern ParserPriority {
    ParserPriority();
    void set(in bit<3> prio);
}

extern CRCPolynomial<T> {
    CRCPolynomial(T coeff, bool reversed, bool msb, bool extended, T init, T xor);
}

extern Hash<W> {
    Hash(HashAlgorithm_t algo);
    Hash(HashAlgorithm_t algo, CRCPolynomial<_> poly);
    W get<D>(in D data);
}

extern Random<W> {
    Random();
    W get();
}

extern IdleTimeout {
    IdleTimeout();
}

extern T max<T>(in T t1, in T t2);
extern T min<T>(in T t1, in T t2);
extern void funnel_shift_right<T>(out T dst, in T src1, in T src2, int shift_amount);
extern void invalidate<T>(in T field);
extern bool is_validated<T>(in T field);
extern T port_metadata_unpack<T>(packet_in pkt);
extern bit<32> sizeInBits<H>(in H h);
extern bit<32> sizeInBytes<H>(in H h);
@noWarn("unused") extern Counter<W, I> {
    Counter(bit<32> size, CounterType_t type, @optional bool true_egress_accounting);
    void count(in I index, @optional in bit<32> adjust_byte_count);
}

@noWarn("unused") extern DirectCounter<W> {
    DirectCounter(CounterType_t type, @optional bool true_egress_accounting);
    void count(@optional in bit<32> adjust_byte_count);
}

extern Meter<I> {
    Meter(bit<32> size, MeterType_t type, @optional bool true_egress_accounting);
    Meter(bit<32> size, MeterType_t type, bit<8> red, bit<8> yellow, bit<8> green, @optional bool true_egress_accounting);
    bit<8> execute(in I index, in MeterColor_t color, @optional in bit<32> adjust_byte_count);
    bit<8> execute(in I index, @optional in bit<32> adjust_byte_count);
}

extern DirectMeter {
    DirectMeter(MeterType_t type, @optional bool true_egress_accounting);
    DirectMeter(MeterType_t type, bit<8> red, bit<8> yellow, bit<8> green, @optional bool true_egress_accounting);
    bit<8> execute(in MeterColor_t color, @optional in bit<32> adjust_byte_count);
    bit<8> execute(@optional in bit<32> adjust_byte_count);
}

extern Lpf<T, I> {
    Lpf(bit<32> size);
    T execute(in T val, in I index);
}

extern DirectLpf<T> {
    DirectLpf();
    T execute(in T val);
}

extern Wred<T, I> {
    Wred(bit<32> size, bit<8> drop_value, bit<8> no_drop_value);
    bit<8> execute(in T val, in I index);
}

extern DirectWred<T> {
    DirectWred(bit<8> drop_value, bit<8> no_drop_value);
    bit<8> execute(in T val);
}

extern Register<T, I> {
    Register(bit<32> size);
    Register(bit<32> size, T initial_value);
    T read(in I index);
    T write(in I index, in T value);
    void clear(in T value, @optional in T busy);
}

extern DirectRegister<T> {
    DirectRegister();
    DirectRegister(T initial_value);
    T read();
    T write(in T value);
    void clear(in T value, @optional in T busy);
}

extern RegisterParam<T> {
    RegisterParam(T initial_value);
    T read();
}

enum MathOp_t {
    MUL,
    SQR,
    SQRT,
    DIV,
    RSQR,
    RSQRT
}

extern MathUnit<T> {
    MathUnit(bool invert, int<2> shift, int<6> scale, tuple<bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>, bit<8>> data);
    MathUnit(MathOp_t op, int factor);
    MathUnit(MathOp_t op, int A, int B);
    T execute(in T x);
}

extern DirectRegisterAction<T, U> {
    DirectRegisterAction(DirectRegister<T> reg);
    U execute(@optional out U rv2, @optional out U rv3, @optional out U rv4);
    @synchronous(execute) abstract void apply(inout T value, @optional out U rv, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    U address(@optional bit<1> subword);
    U predicate(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern DirectRegisterAction2<T, U1, U2> {
    DirectRegisterAction2(DirectRegister<T> reg);
    U1 execute(out U2 rv2);
    @synchronous(execute) abstract void apply(inout T value, out U1 rv1, out U2 rv2);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern DirectRegisterAction3<T, U1, U2, U3> {
    DirectRegisterAction3(DirectRegister<T> reg);
    U1 execute(out U2 rv2, out U3 rv3);
    @synchronous(execute) abstract void apply(inout T value, out U1 rv1, out U2 rv2, out U3 rv3);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern DirectRegisterAction4<T, U1, U2, U3, U4> {
    DirectRegisterAction4(DirectRegister<T> reg);
    U1 execute(out U2 rv2, out U3 rv3, out U4 rv4);
    @synchronous(execute) abstract void apply(inout T value, out U1 rv1, out U2 rv2, out U3 rv3, out U4 rv4);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

@noWarn("unused") extern RegisterAction<T, H, U> {
    RegisterAction(Register<_, _> reg);
    U execute(@optional in H index, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    U execute_log(@optional out U rv2, @optional out U rv3, @optional out U rv4);
    U enqueue(@optional out U rv2, @optional out U rv3, @optional out U rv4);
    U dequeue(@optional out U rv2, @optional out U rv3, @optional out U rv4);
    U push(@optional out U rv2, @optional out U rv3, @optional out U rv4);
    U pop(@optional out U rv2, @optional out U rv3, @optional out U rv4);
    void sweep(@optional in U busy);
    @synchronous(execute , execute_log , enqueue , dequeue , push , pop , sweep) abstract void apply(inout T value, @optional out U rv1, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    @synchronous(enqueue , push) @optional abstract void overflow(@optional inout T value, @optional out U rv1, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    @synchronous(dequeue , pop) @optional abstract void underflow(@optional inout T value, @optional out U rv1, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    U address(@optional bit<1> subword);
    U predicate(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern RegisterAction2<T, H, U1, U2> {
    RegisterAction2(Register<_, _> reg);
    U1 execute(in H index, out U2 rv2);
    U1 execute_log(out U2 rv2);
    U1 enqueue(out U2 rv2);
    U1 dequeue(out U2 rv2);
    U1 push(out U2 rv2);
    U1 pop(out U2 rv2);
    @synchronous(execute , execute_log , enqueue , dequeue , push , pop) abstract void apply(inout T value, out U1 rv1, out U2 rv2);
    @synchronous(enqueue , push) @optional abstract void overflow(@optional inout T value, out U1 rv1, out U2 rv2);
    @synchronous(dequeue , pop) @optional abstract void underflow(@optional inout T value, out U1 rv1, out U2 rv);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern RegisterAction3<T, H, U1, U2, U3> {
    RegisterAction3(Register<_, _> reg);
    U1 execute(in H index, out U2 rv2, out U3 rv3);
    U1 execute_log(out U2 rv2, out U3 rv3);
    U1 enqueue(out U2 rv2, out U3 rv3);
    U1 dequeue(out U2 rv2, out U3 rv3);
    U1 push(out U2 rv2, out U3 rv3);
    U1 pop(out U2 rv2, out U3 rv3);
    @synchronous(execute , execute_log , enqueue , dequeue , push , pop) abstract void apply(inout T value, out U1 rv1, out U2 rv2, out U3 rv3);
    @synchronous(enqueue , push) @optional abstract void overflow(@optional inout T value, out U1 rv1, out U2 rv2, out U3 rv3);
    @synchronous(dequeue , pop) @optional abstract void underflow(@optional inout T value, out U1 rv1, out U2 rv2, out U3 rv3);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern RegisterAction4<T, H, U1, U2, U3, U4> {
    RegisterAction4(Register<_, _> reg);
    U1 execute(in H index, out U2 rv2, out U3 rv3, out U4 rv4);
    U1 execute_log(out U2 rv2, out U3 rv3, out U4 rv4);
    U1 enqueue(out U2 rv2, out U3 rv3, out U4 rv4);
    U1 dequeue(out U2 rv2, out U3 rv3, out U4 rv4);
    U1 push(out U2 rv2, out U3 rv3, out U4 rv4);
    U1 pop(out U2 rv2, out U3 rv3, out U4 rv4);
    @synchronous(execute , execute_log , enqueue , dequeue , push , pop) abstract void apply(inout T value, out U1 rv1, out U2 rv2, out U3 rv3, out U4 rv4);
    @synchronous(enqueue , push) @optional abstract void overflow(@optional inout T value, out U1 rv1, out U2 rv2, out U3 rv3, out U4 rv4);
    @synchronous(dequeue , pop) @optional abstract void underflow(@optional inout T value, out U1 rv1, out U2 rv2, out U3 rv3, out U4 rv4);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern LearnAction<T, H, D, U> {
    LearnAction(Register<T, H> reg);
    U execute(in H index, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    @synchronous(execute) abstract void apply(inout T value, in D digest, in bool learn, @optional out U rv1, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    U address(@optional bit<1> subword);
    U predicate(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern LearnAction2<T, H, D, U1, U2> {
    LearnAction2(Register<T, H> reg);
    U1 execute(in H index, out U2 rv2);
    @synchronous(execute) abstract void apply(inout T value, in D digest, in bool learn, out U1 rv1, out U2 rv2);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern LearnAction3<T, H, D, U1, U2, U3> {
    LearnAction3(Register<T, H> reg);
    U1 execute(in H index, out U2 rv2, out U3 rv3);
    @synchronous(execute) abstract void apply(inout T value, in D digest, in bool learn, out U1 rv1, out U2 rv2, out U3 rv3);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern LearnAction4<T, H, D, U1, U2, U3, U4> {
    LearnAction4(Register<T, H> reg);
    U1 execute(in H index, out U2 rv2, out U3 rv3, out U4 rv4);
    @synchronous(execute) abstract void apply(inout T value, in D digest, in bool learn, out U1 rv1, out U2 rv2, out U3 rv3, out U4 rv4);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<8> max8<I>(in I val, in bit<16> mask, @optional out bit<4> index);
    bit<16> min16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
    bit<16> max16<I>(in I val, in bit<8> mask, @optional out bit<3> index);
}

extern MinMaxAction<T, H, U> {
    MinMaxAction(Register<T, _> reg);
    U execute(@optional in H index, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    @synchronous(execute) abstract void apply(inout bit<128> value, @optional out U rv1, @optional out U rv2, @optional out U rv3, @optional out U rv4);
    U address(@optional bit<1> subword);
    U predicate(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8(in bit<128> val, in bit<16> mask, @optional out bit<4> index, @optional in int<8> postmod);
    bit<8> max8(in bit<128> val, in bit<16> mask, @optional out bit<4> index, @optional in int<8> postmod);
    bit<16> min16(in bit<128> val, in bit<8> mask, @optional out bit<3> index, @optional in int<8> postmod);
    bit<16> max16(in bit<128> val, in bit<8> mask, @optional out bit<3> index, @optional in int<8> postmod);
}

extern MinMaxAction2<T, H, U1, U2> {
    MinMaxAction2(Register<T, _> reg);
    U1 execute(@optional in H index, out U2 rv2);
    @synchronous(execute) abstract void apply(inout bit<128> value, out U1 rv1, out U2 rv2);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8(in bit<128> val, in bit<16> mask, @optional out bit<4> index, @optional in int<9> postmod);
    bit<8> max8(in bit<128> val, in bit<16> mask, @optional out bit<4> index, @optional in int<9> postmod);
    bit<16> min16(in bit<128> val, in bit<8> mask, @optional out bit<3> index, @optional in int<9> postmod);
    bit<16> max16(in bit<128> val, in bit<8> mask, @optional out bit<3> index, @optional in int<9> postmod);
}

extern MinMaxAction3<T, H, U1, U2, U3> {
    MinMaxAction3(Register<T, _> reg);
    U1 execute(@optional in H index, out U2 rv2, out U3 rv3);
    @synchronous(execute) abstract void apply(inout bit<128> value, out U1 rv1, out U2 rv2, out U3 rv3);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8(in bit<128> val, in bit<16> mask, @optional out bit<4> index, @optional in int<9> postmod);
    bit<8> max8(in bit<128> val, in bit<16> mask, @optional out bit<4> index, @optional in int<9> postmod);
    bit<16> min16(in bit<128> val, in bit<8> mask, @optional out bit<3> index, @optional in int<9> postmod);
    bit<16> max16(in bit<128> val, in bit<8> mask, @optional out bit<3> index, @optional in int<9> postmod);
}

extern MinMaxAction4<T, H, U1, U2, U3, U4> {
    MinMaxAction4(Register<T, _> reg);
    U1 execute(@optional in H index, out U2 rv2, out U3 rv3, out U4 rv4);
    @synchronous(execute) abstract void apply(inout bit<128> value, out U1 rv1, out U2 rv2, out U3 rv3, out U4 rv4);
    U address<U>(@optional bit<1> subword);
    U predicate<U>(@optional in bool cmp0, @optional in bool cmp1, @optional in bool cmp2, @optional in bool cmp3);
    bit<8> min8(in bit<128> val, in bit<16> mask, @optional out bit<4> index, @optional in int<9> postmod);
    bit<8> max8(in bit<128> val, in bit<16> mask, @optional out bit<4> index, @optional in int<9> postmod);
    bit<16> min16(in bit<128> val, in bit<8> mask, @optional out bit<3> index, @optional in int<9> postmod);
    bit<16> max16(in bit<128> val, in bit<8> mask, @optional out bit<3> index, @optional in int<9> postmod);
}

extern ActionProfile {
    ActionProfile(bit<32> size);
}

extern ActionSelector {
    ActionSelector(ActionProfile action_profile, Hash<_> hash, SelectorMode_t mode, bit<32> max_group_size, bit<32> num_groups);
    ActionSelector(ActionProfile action_profile, Hash<_> hash, SelectorMode_t mode, Register<bit<1>, _> reg, bit<32> max_group_size, bit<32> num_groups);
    @deprecated("ActionSelector must be specified with an associated ActionProfile") ActionSelector(bit<32> size, Hash<_> hash, SelectorMode_t mode);
    @deprecated("ActionSelector must be specified with an associated ActionProfile") ActionSelector(bit<32> size, Hash<_> hash, SelectorMode_t mode, Register<bit<1>, _> reg);
}

extern SelectorAction {
    SelectorAction(ActionSelector sel);
    bit<1> execute(@optional in bit<32> index);
    @synchronous(execute) abstract void apply(inout bit<1> value, @optional out bit<1> rv);
}

extern Mirror {
    @deprecated("Mirror must be specified with the value of the mirror_type instrinsic metadata") Mirror();
    Mirror(MirrorType_t mirror_type);
    void emit(in MirrorId_t session_id);
    void emit<T>(in MirrorId_t session_id, in T hdr);
}

extern Resubmit {
    @deprecated("Resubmit must be specified with the value of the resubmit_type instrinsic metadata") Resubmit();
    Resubmit(ResubmitType_t resubmit_type);
    void emit();
    void emit<T>(in T hdr);
}

extern Digest<T> {
    @deprecated("Digest must be specified with the value of the digest_type instrinsic metadata") Digest();
    Digest(DigestType_t digest_type);
    void pack(in T data);
}

extern Pktgen {
    Pktgen();
    void emit<T>(in T hdr);
}

extern Atcam {
    Atcam(@optional bit<32> number_partitions);
}

extern Alpm {
    Alpm(@optional bit<32> number_partitions, @optional bit<32> subtrees_per_partition, @optional bit<32> atcam_subset_width, @optional bit<32> shift_granularity);
}

parser IngressParserT<H, M>(packet_in pkt, out H hdr, out M ig_md, out ingress_intrinsic_metadata_t ig_intr_md, @optional out ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm, @optional out ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr);
parser EgressParserT<H, M>(packet_in pkt, out H hdr, out M eg_md, out egress_intrinsic_metadata_t eg_intr_md, @optional out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, @optional out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr);
control IngressT<H, M>(inout H hdr, inout M ig_md, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr, inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr, inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm, @optional in ghost_intrinsic_metadata_t gh_intr_md);
control GhostT(in ghost_intrinsic_metadata_t gh_intr_md);
control EgressT<H, M>(inout H hdr, inout M eg_md, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr, inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport);
control IngressDeparserT<H, M>(packet_out pkt, inout H hdr, in M metadata, in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr, @optional in ingress_intrinsic_metadata_t ig_intr_md);
control EgressDeparserT<H, M>(packet_out pkt, inout H hdr, in M metadata, in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr, @optional in egress_intrinsic_metadata_t eg_intr_md, @optional in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr);
package Pipeline<IH, IM, EH, EM>(IngressParserT<IH, IM> ingress_parser, IngressT<IH, IM> ingress, IngressDeparserT<IH, IM> ingress_deparser, EgressParserT<EH, EM> egress_parser, EgressT<EH, EM> egress, EgressDeparserT<EH, EM> egress_deparser, @optional GhostT ghost);
@pkginfo(arch = "T2NA" , version = "1.0.1") package Switch<IH0, IM0, EH0, EM0, IH1, IM1, EH1, EM1, IH2, IM2, EH2, EM2, IH3, IM3, EH3, EM3>(Pipeline<IH0, IM0, EH0, EM0> pipe0, @optional Pipeline<IH1, IM1, EH1, EM1> pipe1, @optional Pipeline<IH2, IM2, EH2, EM2> pipe2, @optional Pipeline<IH3, IM3, EH3, EM3> pipe3);
const bit<32> VLAN_TABLE_SIZE = 4096;
const bit<32> BD_FLOOD_TABLE_SIZE = VLAN_TABLE_SIZE * 4;
const bit<32> PORT_VLAN_TABLE_SIZE = 1024;
const bit<32> DOUBLE_TAG_TABLE_SIZE = 4096;
const bit<32> BD_TABLE_SIZE = 5120;
const bit<32> MAC_TABLE_SIZE = 16384;
const bit<32> IPV4_HOST_TABLE_SIZE = 256 * 1024;
const bit<32> IPV4_LPM_TABLE_SIZE = 256 * 1024;
const bit<32> IPV6_HOST_TABLE_SIZE = 4 * 1024;
const bit<32> IPV6_HOST64_TABLE_SIZE = 256 * 1024;
const bit<32> IPV6_LPM_TABLE_SIZE = 512;
const bit<32> IPV6_LPM64_TABLE_SIZE = 256 * 1024;
const bit<32> ECMP_GROUP_TABLE_SIZE = 1024;
const bit<32> ECMP_SELECT_TABLE_SIZE = 65536;
const bit<32> NEXTHOP_TABLE_SIZE = 1 << 16;
const bit<32> TUNNEL_NEXTHOP_TABLE_SIZE = 32 * 1024;
const bit<32> TUNNEL_OBJECT_SIZE = 1 << 8;
const bit<32> TUNNEL_ENCAP_IPV4_SIZE = 32 * 1024;
const bit<32> TUNNEL_ENCAP_IPV6_SIZE = 32 * 1024;
const bit<32> TUNNEL_ENCAP_IP_SIZE = TUNNEL_ENCAP_IPV4_SIZE + TUNNEL_ENCAP_IPV6_SIZE;
const bit<32> RID_TABLE_SIZE = 16384;
const bit<32> INGRESS_IPV4_ACL_TABLE_SIZE = 2 * 1024;
const bit<32> INGRESS_IPV6_ACL_TABLE_SIZE = 1024;
const bit<32> INGRESS_IP_MIRROR_ACL_TABLE_SIZE = 1024;
const bit<32> EGRESS_IPV4_ACL_TABLE_SIZE = 1024;
const bit<32> EGRESS_IPV6_ACL_TABLE_SIZE = 512;
const bit<32> EGRESS_IPV4_MIRROR_ACL_TABLE_SIZE = 512;
const bit<32> EGRESS_IPV6_MIRROR_ACL_TABLE_SIZE = 512;
const bit<32> FLOW_NAT_TABLE_SIZE = 4 * 1024;
const bit<32> FLOW_NAPT_TABLE_SIZE = 4 * 1024;
const bit<32> DNAPT_TABLE_SIZE = 32 * 1024;
const bit<32> DNAT_POOL_TABLE_SIZE = DNAPT_TABLE_SIZE;
const bit<32> DNAT_TABLE_SIZE = 4 * 1024;
const bit<32> INGRESS_NAT_REWRITE_TABLE_SIZE = 1024;
const bit<32> SNAPT_TABLE_SIZE = 32 * 1024;
const bit<32> SNAT_TABLE_SIZE = 4 * 1024;
const bit<32> BFD_SESSION_SIZE = 4096;
const bit<32> BFD_PER_PIPE_SESSION_SIZE = 1024;
const bit<32> EGRESS_QUEUE_TABLE_SIZE = 2048;
typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<12> vlan_id_t;
const int PAD_SIZE = 32;
const int MIN_SIZE = 64;
header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16>    ether_type;
}

header vlan_tag_h {
    bit<3>    pcp;
    bit<1>    dei;
    vlan_id_t vid;
    bit<16>   ether_type;
}

header mpls_h {
    bit<20> label;
    bit<3>  exp;
    bit<1>  bos;
    bit<8>  ttl;
}

header ipv4_h {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     total_len;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header router_alert_option_h {
    bit<8>  type;
    bit<8>  length;
    bit<16> value;
}

header ipv4_option_h {
    bit<8>  type;
    bit<8>  length;
    bit<16> value;
}

header ipv6_h {
    bit<4>      version;
    bit<8>      traffic_class;
    bit<20>     flow_label;
    bit<16>     payload_len;
    bit<8>      next_hdr;
    bit<8>      hop_limit;
    ipv6_addr_t src_addr;
    ipv6_addr_t dst_addr;
}

header tcp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

header icmp_h {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
}

header igmp_h {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
}

header arp_h {
    bit<16> hw_type;
    bit<16> proto_type;
    bit<8>  hw_addr_len;
    bit<8>  proto_addr_len;
    bit<16> opcode;
}

header rocev2_bth_h {
    bit<8>  opcodee;
    bit<1>  se;
    bit<1>  migration_req;
    bit<2>  pad_count;
    bit<4>  transport_version;
    bit<16> partition_key;
    bit<1>  f_res1;
    bit<1>  b_res1;
    bit<6>  reserved;
    bit<24> dst_qp;
    bit<1>  ack_req;
    bit<7>  reserved2;
}

header fcoe_fc_h {
    bit<4>   version;
    bit<100> reserved;
    bit<8>   sof;
    bit<8>   r_ctl;
    bit<24>  d_id;
    bit<8>   cs_ctl;
    bit<24>  s_id;
    bit<8>   type;
    bit<24>  f_ctl;
    bit<8>   seq_id;
    bit<8>   df_ctl;
    bit<16>  seq_cnt;
    bit<16>  ox_id;
    bit<16>  rx_id;
}

header ipv6_srh_h {
    bit<8>  next_hdr;
    bit<8>  hdr_ext_len;
    bit<8>  routing_type;
    bit<8>  seg_left;
    bit<8>  last_entry;
    bit<8>  flags;
    bit<16> tag;
}

header ipv6_segment_h {
    bit<128> sid;
}

header vxlan_h {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved2;
}

header vxlan_gpe_h {
    bit<8>  flags;
    bit<16> reserved;
    bit<24> vni;
    bit<8>  reserved2;
}

header gre_h {
    bit<16> flags_version;
    bit<16> proto;
}

header nvgre_h {
    bit<32> vsid_flowid;
}

header erspan_h {
    bit<16> version_vlan;
    bit<16> session_id;
}

header erspan_type2_h {
    bit<32> index;
}

header erspan_type3_h {
    bit<32> timestamp;
    bit<32> ft_d_other;
}

header erspan_platform_h {
    bit<6>  id;
    bit<58> info;
}

header gtpu_h {
    bit<3>  version;
    bit<1>  p;
    bit<1>  t;
    bit<3>  spare0;
    bit<8>  mesg_type;
    bit<16> mesg_len;
    bit<32> teid;
    bit<24> seq_num;
    bit<8>  spare1;
}

header geneve_h {
    bit<2>  version;
    bit<6>  opt_len;
    bit<1>  oam;
    bit<1>  critical;
    bit<6>  reserved;
    bit<16> proto_type;
    bit<24> vni;
    bit<8>  reserved2;
}

header geneve_option_h {
    bit<16> opt_class;
    bit<8>  opt_type;
    bit<3>  reserved;
    bit<5>  opt_len;
}

header bfd_h {
    bit<3>  version;
    bit<5>  diag;
    bit<8>  flags;
    bit<8>  detect_multi;
    bit<8>  len;
    bit<32> my_discriminator;
    bit<32> your_discriminator;
    bit<32> desired_min_tx_interval;
    bit<32> req_min_rx_interval;
    bit<32> req_min_echo_rx_interval;
}

header dtel_report_v05_h {
    bit<4>  version;
    bit<4>  next_proto;
    bit<3>  d_q_f;
    bit<15> reserved;
    bit<6>  hw_id;
    bit<32> seq_number;
    bit<32> timestamp;
    bit<32> switch_id;
}

header dtel_report_base_h {
    bit<7>   pad0;
    PortId_t ingress_port;
    bit<7>   pad1;
    PortId_t egress_port;
    bit<1>   pad2;
    bit<7>   queue_id;
}

header dtel_drop_report_h {
    bit<8>  drop_reason;
    bit<16> reserved;
}

header dtel_switch_local_report_h {
    bit<5>  pad3;
    bit<19> queue_occupancy;
    bit<32> timestamp;
}

header dtel_report_v10_h {
    bit<4>  version;
    bit<4>  length;
    bit<3>  next_proto;
    bit<6>  metadata_bits;
    bit<6>  reserved;
    bit<3>  d_q_f;
    bit<6>  hw_id;
    bit<32> switch_id;
    bit<32> seq_number;
    bit<32> timestamp;
}

header dtel_report_v20_h {
    bit<4>  version;
    bit<4>  hw_id;
    bit<24> seq_number;
    bit<32> switch_id;
    bit<16> report_length;
    bit<8>  md_length;
    bit<3>  d_q_f;
    bit<5>  reserved;
    bit<16> rep_md_bits;
    bit<16> domain_specific_id;
    bit<16> ds_md_bits;
    bit<16> ds_md_status;
}

header dtel_metadata_1_h {
    bit<7>   pad0;
    PortId_t ingress_port;
    bit<7>   pad1;
    PortId_t egress_port;
}

header dtel_metadata_2_h {
    bit<32> hop_latency;
}

header dtel_metadata_3_h {
    bit<1>  pad2;
    bit<7>  queue_id;
    bit<5>  pad3;
    bit<19> queue_occupancy;
}

header dtel_metadata_4_h {
    bit<16> pad;
    bit<48> ingress_timestamp;
}

header dtel_metadata_5_h {
    bit<16> pad;
    bit<48> egress_timestamp;
}

header dtel_report_metadata_15_h {
    bit<1>  pad;
    bit<7>  queue_id;
    bit<8>  drop_reason;
    bit<16> reserved;
}

header fabric_h {
    bit<8> reserved;
    bit<3> color;
    bit<5> qos;
    bit<8> reserved2;
}

header cpu_h {
    bit<1>  tx_bypass;
    bit<1>  capture_ts;
    bit<1>  reserved;
    bit<5>  egress_queue;
    bit<16> ingress_port;
    bit<16> port_lag_index;
    bit<16> ingress_bd;
    bit<16> reason_code;
    bit<16> ether_type;
}

header sfc_pause_h {
    bit<8>  version;
    bit<8>  dscp;
    bit<16> duration_us;
}

header padding_112b_h {
    bit<112> pad_0;
}

header padding_96b_h {
    bit<96> pad;
}

header pfc_h {
    bit<16> opcode;
    bit<8>  reserved_zero;
    bit<8>  class_enable_vec;
    bit<16> tstamp0;
    bit<16> tstamp1;
    bit<16> tstamp2;
    bit<16> tstamp3;
    bit<16> tstamp4;
    bit<16> tstamp5;
    bit<16> tstamp6;
    bit<16> tstamp7;
}

header timestamp_h {
    bit<48> timestamp;
}

header pad_h {
    bit<(PAD_SIZE * 8)> data;
}

const bit<32> MIN_TABLE_SIZE = 512;
const bit<32> LAG_TABLE_SIZE = 1024;
const bit<32> LAG_GROUP_TABLE_SIZE = 256;
const bit<32> LAG_MAX_MEMBERS_PER_GROUP = 64;
const bit<32> LAG_SELECTOR_TABLE_SIZE = 16384;
const bit<32> DTEL_GROUP_TABLE_SIZE = 4;
const bit<32> DTEL_MAX_MEMBERS_PER_GROUP = 64;
const bit<32> DTEL_SELECTOR_TABLE_SIZE = 256;
const bit<32> IPV4_DST_VTEP_TABLE_SIZE = 512;
const bit<32> IPV6_DST_VTEP_TABLE_SIZE = 512;
const bit<32> VNI_MAPPING_TABLE_SIZE = 1024;
const bit<32> BD_TO_VNI_MAPPING_SIZE = 4096;
@pa_container_size("ingress" , "local_md.lkp.ip_dst_addr" , 32) @pa_container_size("ingress" , "_ipv4_lpm_partition_key" , 32) @pa_container_size("ingress" , "hdr.ipv4.dst_addr" , 32) @pa_container_size("ingress" , "hdr.ipv6.dst_addr" , 32) @pa_container_size("ingress" , "hdr.inner_ipv4.dst_addr" , 32) @pa_container_size("ingress" , "hdr.inner_ipv6.dst_addr" , 32) typedef bit<32> switch_uint32_t;
typedef bit<16> switch_uint16_t;
typedef bit<8> switch_uint8_t;
typedef PortId_t switch_port_t;
const switch_port_t SWITCH_PORT_INVALID = 9w0x1ff;
typedef bit<7> switch_port_padding_t;
typedef QueueId_t switch_qid_t;
typedef ReplicationId_t switch_rid_t;
const switch_rid_t SWITCH_RID_DEFAULT = 0xffff;
typedef bit<3> switch_ingress_cos_t;
typedef bit<3> switch_digest_type_t;
const switch_digest_type_t SWITCH_DIGEST_TYPE_INVALID = 0;
const switch_digest_type_t SWITCH_DIGEST_TYPE_MAC_LEARNING = 1;
typedef bit<16> switch_ifindex_t;
typedef bit<16> switch_port_lag_index_t;
const switch_port_lag_index_t SWITCH_FLOOD = 0x3ff;
typedef bit<8> switch_isolation_group_t;
typedef bit<16> switch_bd_t;
const switch_bd_t SWITCH_BD_DEFAULT_VRF = 4097;
const bit<32> VRF_TABLE_SIZE = 1 << 10;
typedef bit<10> switch_vrf_t;
const switch_vrf_t SWITCH_DEFAULT_VRF = 1;
typedef bit<16> switch_nexthop_t;
typedef bit<10> switch_user_metadata_t;
typedef bit<32> switch_hash_t;
typedef bit<16> switch_ecmp_hash_t;
typedef bit<128> srv6_sid_t;
typedef bit<16> switch_xid_t;
typedef L2ExclusionId_t switch_yid_t;
typedef bit<24> switch_ig_port_lag_label_t;
typedef bit<20> switch_eg_port_lag_label_t;
typedef bit<16> switch_bd_label_t;
typedef bit<16> switch_mtu_t;
typedef bit<12> switch_stats_index_t;
typedef bit<8> switch_ports_group_label_t;
typedef bit<16> switch_cpu_reason_t;
const switch_cpu_reason_t SWITCH_CPU_REASON_PTP = 0x8;
const switch_cpu_reason_t SWITCH_CPU_REASON_BFD = 0x9;
typedef bit<8> switch_fib_label_t;
struct switch_cpu_port_value_set_t {
    bit<16>       ether_type;
    switch_port_t port;
}

typedef bit<2> bfd_pkt_action_t;
const bfd_pkt_action_t BFD_PKT_ACTION_DROP = 0x0;
const bfd_pkt_action_t BFD_PKT_ACTION_TIMEOUT = 0x1;
const bfd_pkt_action_t BFD_PKT_ACTION_NORMAL = 0x2;
typedef bit<8> bfd_multiplier_t;
typedef bit<12> bfd_session_t;
typedef bit<4> bfd_pipe_t;
typedef bit<8> bfd_timer_t;
struct switch_bfd_metadata_t {
    bfd_multiplier_t tx_mult;
    bfd_multiplier_t rx_mult;
    bfd_pkt_action_t pkt_action;
    bfd_pipe_t       pktgen_pipe;
    bfd_session_t    session_id;
    bit<1>           tx_timer_expired;
    bit<1>           session_offload;
    bit<1>           pkt_tx;
}

typedef bit<8> switch_drop_reason_t;
const switch_drop_reason_t SWITCH_DROP_REASON_UNKNOWN = 0;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO = 10;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST = 11;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO = 12;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_ETHERNET_MISS = 13;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK = 17;
const switch_drop_reason_t SWITCH_DROP_REASON_SRC_MAC_ZERO = 14;
const switch_drop_reason_t SWITCH_DROP_REASON_SRC_MAC_MULTICAST = 15;
const switch_drop_reason_t SWITCH_DROP_REASON_DST_MAC_ZERO = 16;
const switch_drop_reason_t SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_UCAST = 18;
const switch_drop_reason_t SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_BCAST = 19;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_VERSION_INVALID = 25;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO = 26;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_SRC_MULTICAST = 27;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_SRC_LOOPBACK = 28;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_MISS = 29;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_IHL_INVALID = 30;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_INVALID_CHECKSUM = 31;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_DST_LOOPBACK = 32;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_SRC_UNSPECIFIED = 33;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_SRC_CLASS_E = 34;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_VERSION_INVALID = 40;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_TTL_ZERO = 41;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_MULTICAST = 42;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_LOOPBACK = 43;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_IHL_INVALID = 44;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_INVALID_CHECKSUM = 45;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_CLASS_E = 46;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_DST_LINK_LOCAL = 47;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_LINK_LOCAL = 48;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_DST_UNSPECIFIED = 49;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_UNSPECIFIED = 50;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_LPM4_MISS = 51;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_LPM6_MISS = 52;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_BLACKHOLE_ROUTE = 53;
const switch_drop_reason_t SWITCH_DROP_REASON_L3_PORT_RMAC_MISS = 54;
const switch_drop_reason_t SWITCH_DROP_REASON_PORT_VLAN_MAPPING_MISS = 55;
const switch_drop_reason_t SWITCH_DROP_REASON_STP_STATE_LEARNING = 56;
const switch_drop_reason_t SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING = 57;
const switch_drop_reason_t SWITCH_DROP_REASON_SAME_IFINDEX = 58;
const switch_drop_reason_t SWITCH_DROP_REASON_MULTICAST_SNOOPING_ENABLED = 59;
const switch_drop_reason_t SWITCH_DROP_REASON_IN_L3_EGRESS_LINK_DOWN = 60;
const switch_drop_reason_t SWITCH_DROP_REASON_MTU_CHECK_FAIL = 70;
const switch_drop_reason_t SWITCH_DROP_REASON_TRAFFIC_MANAGER = 71;
const switch_drop_reason_t SWITCH_DROP_REASON_STORM_CONTROL = 72;
const switch_drop_reason_t SWITCH_DROP_REASON_WRED = 73;
const switch_drop_reason_t SWITCH_DROP_REASON_INGRESS_PORT_METER = 75;
const switch_drop_reason_t SWITCH_DROP_REASON_INGRESS_ACL_METER = 76;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_PORT_METER = 77;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_ACL_METER = 78;
const switch_drop_reason_t SWITCH_DROP_REASON_ACL_DROP = 80;
const switch_drop_reason_t SWITCH_DROP_REASON_RACL_DENY = 81;
const switch_drop_reason_t SWITCH_DROP_REASON_URPF_CHECK_FAIL = 82;
const switch_drop_reason_t SWITCH_DROP_REASON_IPSG_MISS = 83;
const switch_drop_reason_t SWITCH_DROP_REASON_IFINDEX = 84;
const switch_drop_reason_t SWITCH_DROP_REASON_CPU_COLOR_YELLOW = 85;
const switch_drop_reason_t SWITCH_DROP_REASON_CPU_COLOR_RED = 86;
const switch_drop_reason_t SWITCH_DROP_REASON_STORM_CONTROL_COLOR_YELLOW = 87;
const switch_drop_reason_t SWITCH_DROP_REASON_STORM_CONTROL_COLOR_RED = 88;
const switch_drop_reason_t SWITCH_DROP_REASON_L2_MISS_UNICAST = 89;
const switch_drop_reason_t SWITCH_DROP_REASON_L2_MISS_MULTICAST = 90;
const switch_drop_reason_t SWITCH_DROP_REASON_L2_MISS_BROADCAST = 91;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_ACL_DROP = 92;
const switch_drop_reason_t SWITCH_DROP_REASON_NEXTHOP = 93;
const switch_drop_reason_t SWITCH_DROP_REASON_NON_IP_ROUTER_MAC = 94;
const switch_drop_reason_t SWITCH_DROP_REASON_L3_IPV4_DISABLE = 99;
const switch_drop_reason_t SWITCH_DROP_REASON_L3_IPV6_DISABLE = 100;
const switch_drop_reason_t SWITCH_DROP_REASON_INGRESS_PFC_WD_DROP = 101;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_PFC_WD_DROP = 102;
const switch_drop_reason_t SWITCH_DROP_REASON_MPLS_LABEL_DROP = 103;
const switch_drop_reason_t SWITCH_DROP_REASON_SRV6_MY_SID_DROP = 104;
const switch_drop_reason_t SWITCH_DROP_REASON_PORT_ISOLATION_DROP = 105;
const switch_drop_reason_t SWITCH_DROP_REASON_DMAC_RESERVED = 106;
const switch_drop_reason_t SWITCH_DROP_REASON_NON_ROUTABLE = 107;
const switch_drop_reason_t SWITCH_DROP_REASON_MPLS_DISABLE = 108;
const switch_drop_reason_t SWITCH_DROP_REASON_BFD = 109;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_STP_STATE_BLOCKING = 110;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_MULTICAST_DMAC_MISMATCH = 111;
const switch_drop_reason_t SWITCH_DROP_REASON_SIP_BC = 112;
const switch_drop_reason_t SWITCH_DROP_REASON_IPV6_MC_SCOPE0 = 113;
const switch_drop_reason_t SWITCH_DROP_REASON_IPV6_MC_SCOPE1 = 114;
const switch_drop_reason_t SWITCH_DROP_REASON_ACL_DENY = 115;
typedef bit<1> switch_port_type_t;
const switch_port_type_t SWITCH_PORT_TYPE_NORMAL = 0;
const switch_port_type_t SWITCH_PORT_TYPE_CPU = 1;
typedef bit<2> switch_ip_type_t;
const switch_ip_type_t SWITCH_IP_TYPE_NONE = 0;
const switch_ip_type_t SWITCH_IP_TYPE_IPV4 = 1;
const switch_ip_type_t SWITCH_IP_TYPE_IPV6 = 2;
const switch_ip_type_t SWITCH_IP_TYPE_MPLS = 3;
typedef bit<2> switch_ip_frag_t;
const switch_ip_frag_t SWITCH_IP_FRAG_NON_FRAG = 0b0;
const switch_ip_frag_t SWITCH_IP_FRAG_HEAD = 0b10;
const switch_ip_frag_t SWITCH_IP_FRAG_NON_HEAD = 0b11;
typedef bit<2> switch_packet_action_t;
const switch_packet_action_t SWITCH_PACKET_ACTION_PERMIT = 0b0;
const switch_packet_action_t SWITCH_PACKET_ACTION_DROP = 0b1;
const switch_packet_action_t SWITCH_PACKET_ACTION_COPY = 0b10;
const switch_packet_action_t SWITCH_PACKET_ACTION_TRAP = 0b11;
typedef bit<16> switch_ingress_bypass_t;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_L2 = 16w0x1 << 0;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_L3 = 16w0x1 << 1;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_ACL = 16w0x1 << 2;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_SYSTEM_ACL = 16w0x1 << 3;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_QOS = 16w0x1 << 4;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_METER = 16w0x1 << 5;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_STORM_CONTROL = 16w0x1 << 6;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_STP = 16w0x1 << 7;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_SMAC = 16w0x1 << 8;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_NAT = 16w0x1 << 9;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_ROUTING_CHECK = 16w0x1 << 10;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_PV = 16w0x1 << 11;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_BFD_TX = 16w0x1 << 15;
const switch_ingress_bypass_t SWITCH_INGRESS_BYPASS_ALL = 16w0x7fff;
typedef bit<16> switch_pkt_length_t;
typedef bit<8> switch_pkt_src_t;
const switch_pkt_src_t SWITCH_PKT_SRC_BRIDGED = 0;
const switch_pkt_src_t SWITCH_PKT_SRC_CLONED_INGRESS = 1;
const switch_pkt_src_t SWITCH_PKT_SRC_CLONED_EGRESS = 2;
const switch_pkt_src_t SWITCH_PKT_SRC_DEFLECTED = 3;
const switch_pkt_src_t SWITCH_PKT_SRC_CLONED_EGRESS_IN_PKT = 4;
typedef bit<2> switch_pkt_color_t;
const switch_pkt_color_t SWITCH_METER_COLOR_GREEN = 0;
const switch_pkt_color_t SWITCH_METER_COLOR_YELLOW = 1;
const switch_pkt_color_t SWITCH_METER_COLOR_RED = 3;
typedef bit<2> switch_pkt_type_t;
const switch_pkt_type_t SWITCH_PKT_TYPE_UNICAST = 0;
const switch_pkt_type_t SWITCH_PKT_TYPE_MULTICAST = 1;
const switch_pkt_type_t SWITCH_PKT_TYPE_BROADCAST = 2;
typedef bit<8> switch_l4_port_label_t;
typedef bit<4> switch_etype_label_t;
typedef bit<8> switch_mac_addr_label_t;
typedef bit<2> switch_stp_state_t;
const switch_stp_state_t SWITCH_STP_STATE_FORWARDING = 0;
const switch_stp_state_t SWITCH_STP_STATE_BLOCKING = 1;
const switch_stp_state_t SWITCH_STP_STATE_LEARNING = 2;
typedef bit<10> switch_stp_group_t;
struct switch_stp_metadata_t {
    switch_stp_group_t group;
    switch_stp_state_t state_;
}

typedef bit<2> switch_nexthop_type_t;
const switch_nexthop_type_t SWITCH_NEXTHOP_TYPE_IP = 0;
const switch_nexthop_type_t SWITCH_NEXTHOP_TYPE_MPLS = 1;
const switch_nexthop_type_t SWITCH_NEXTHOP_TYPE_TUNNEL_ENCAP = 2;
typedef bit<8> switch_sflow_id_t;
const switch_sflow_id_t SWITCH_SFLOW_INVALID_ID = 8w0xff;
struct switch_sflow_metadata_t {
    switch_sflow_id_t session_id;
    bit<1>            sample_packet;
}

typedef bit<8> switch_hostif_trap_t;
typedef bit<8> switch_copp_meter_id_t;
typedef bit<10> switch_port_meter_id_t;
typedef bit<10> switch_acl_meter_id_t;
typedef bit<8> switch_mirror_meter_id_t;
typedef bit<2> switch_myip_type_t;
const switch_myip_type_t SWITCH_MYIP_NONE = 0;
const switch_myip_type_t SWITCH_MYIP = 1;
const switch_myip_type_t SWITCH_MYIP_SUBNET = 2;
typedef bit<2> switch_fwd_type_t;
const switch_fwd_type_t SWITCH_FWD_TYPE_NONE = 0;
const switch_fwd_type_t SWITCH_FWD_TYPE_L2 = 1;
const switch_fwd_type_t SWITCH_FWD_TYPE_L3 = 2;
const switch_fwd_type_t SWITCH_FWD_TYPE_MC = 3;
typedef bit<16> switch_fwd_idx_t;
typedef bit<2> switch_qos_trust_mode_t;
const switch_qos_trust_mode_t SWITCH_QOS_TRUST_MODE_UNTRUSTED = 0;
const switch_qos_trust_mode_t SWITCH_QOS_TRUST_MODE_TRUST_DSCP = 1;
const switch_qos_trust_mode_t SWITCH_QOS_TRUST_MODE_TRUST_PCP = 2;
typedef bit<6> switch_tc_t;
typedef bit<3> switch_cos_t;
typedef bit<11> switch_etrap_index_t;
struct switch_qos_metadata_t {
    switch_qos_trust_mode_t trust_mode;
    switch_tc_t             tc;
    switch_pkt_color_t      color;
    switch_pkt_color_t      acl_meter_color;
    switch_pkt_color_t      storm_control_color;
    switch_port_meter_id_t  port_meter_index;
    switch_acl_meter_id_t   acl_meter_index;
    switch_qid_t            qid;
    switch_ingress_cos_t    icos;
    bit<19>                 qdepth;
    switch_etrap_index_t    etrap_index;
    switch_pkt_color_t      etrap_color;
    switch_tc_t             etrap_tc;
    bit<1>                  etrap_state;
    bit<2>                  outer_ecn;
    bit<3>                  pcp;
}

typedef bit<1> switch_learning_mode_t;
const switch_learning_mode_t SWITCH_LEARNING_MODE_DISABLED = 0;
const switch_learning_mode_t SWITCH_LEARNING_MODE_LEARN = 1;
struct switch_learning_digest_t {
    switch_bd_t             bd;
    switch_port_lag_index_t port_lag_index;
    mac_addr_t              src_addr;
}

struct switch_learning_metadata_t {
    switch_learning_mode_t   bd_mode;
    switch_learning_mode_t   port_mode;
    switch_learning_digest_t digest;
}

typedef bit<2> switch_multicast_mode_t;
const switch_multicast_mode_t SWITCH_MULTICAST_MODE_NONE = 0;
const switch_multicast_mode_t SWITCH_MULTICAST_MODE_PIM_SM = 1;
const switch_multicast_mode_t SWITCH_MULTICAST_MODE_PIM_BIDIR = 2;
typedef MulticastGroupId_t switch_mgid_t;
typedef bit<16> switch_multicast_rpf_group_t;
struct switch_multicast_metadata_t {
    switch_mgid_t                id;
    bit<2>                       mode;
    switch_multicast_rpf_group_t rpf_group;
    bit<1>                       hit;
}

typedef bit<2> switch_urpf_mode_t;
const switch_urpf_mode_t SWITCH_URPF_MODE_NONE = 0;
const switch_urpf_mode_t SWITCH_URPF_MODE_LOOSE = 1;
const switch_urpf_mode_t SWITCH_URPF_MODE_STRICT = 2;
typedef bit<10> switch_wred_index_t;
typedef bit<2> switch_ecn_codepoint_t;
const switch_ecn_codepoint_t SWITCH_ECN_CODEPOINT_NON_ECT = 0b0;
const switch_ecn_codepoint_t SWITCH_ECN_CODEPOINT_ECT0 = 0b10;
const switch_ecn_codepoint_t SWITCH_ECN_CODEPOINT_ECT1 = 0b1;
const switch_ecn_codepoint_t SWITCH_ECN_CODEPOINT_CE = 0b11;
const switch_ecn_codepoint_t NON_ECT = 0b0;
const switch_ecn_codepoint_t ECT0 = 0b10;
const switch_ecn_codepoint_t ECT1 = 0b1;
const switch_ecn_codepoint_t CE = 0b11;
typedef MirrorId_t switch_mirror_session_t;
const switch_mirror_session_t SWITCH_MIRROR_SESSION_CPU = 250;
typedef bit<8> switch_mirror_type_t;
struct switch_mirror_metadata_t {
    switch_pkt_src_t         src;
    switch_mirror_type_t     type;
    switch_mirror_session_t  session_id;
    switch_mirror_meter_id_t meter_index;
}

header switch_port_mirror_metadata_h {
    switch_pkt_src_t        src;
    switch_mirror_type_t    type;
    bit<32>                 timestamp;
    switch_mirror_session_t session_id;
    switch_port_padding_t   _pad1;
    switch_port_t           port;
}

header switch_cpu_mirror_metadata_h {
    switch_pkt_src_t        src;
    switch_mirror_type_t    type;
    switch_port_padding_t   _pad1;
    switch_port_t           port;
    switch_bd_t             bd;
    switch_port_lag_index_t port_lag_index;
    switch_cpu_reason_t     reason_code;
}

typedef bit<1> switch_tunnel_mode_t;
const switch_tunnel_mode_t SWITCH_TUNNEL_MODE_PIPE = 0;
const switch_tunnel_mode_t SWITCH_TUNNEL_MODE_UNIFORM = 1;
const switch_tunnel_mode_t SWITCH_ECN_MODE_STANDARD = 0;
const switch_tunnel_mode_t SWITCH_ECN_MODE_COPY_FROM_OUTER = 1;
typedef bit<4> switch_tunnel_type_t;
const switch_tunnel_type_t SWITCH_INGRESS_TUNNEL_TYPE_NONE = 0;
const switch_tunnel_type_t SWITCH_INGRESS_TUNNEL_TYPE_VXLAN = 1;
const switch_tunnel_type_t SWITCH_INGRESS_TUNNEL_TYPE_IPINIP = 2;
const switch_tunnel_type_t SWITCH_INGRESS_TUNNEL_TYPE_GRE = 3;
const switch_tunnel_type_t SWITCH_INGRESS_TUNNEL_TYPE_NVGRE = 4;
const switch_tunnel_type_t SWITCH_INGRESS_TUNNEL_TYPE_MPLS = 5;
const switch_tunnel_type_t SWITCH_INGRESS_TUNNEL_TYPE_SRV6 = 6;
const switch_tunnel_type_t SWITCH_INGRESS_TUNNEL_TYPE_NVGRE_ST = 7;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_NONE = 0;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_IPV4_VXLAN = 1;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_IPV6_VXLAN = 2;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_IPV4_IPINIP = 3;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_IPV6_IPINIP = 4;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_IPV4_NVGRE = 5;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_IPV6_NVGRE = 6;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_MPLS = 7;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_SRV6_ENCAP = 8;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_SRV6_INSERT = 9;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_IPV4_GRE = 10;
const switch_tunnel_type_t SWITCH_EGRESS_TUNNEL_TYPE_IPV6_GRE = 11;
enum switch_tunnel_term_mode_t {
    P2P,
    P2MP
}

typedef bit<8> switch_tunnel_index_t;
typedef bit<4> switch_tunnel_mapper_index_t;
typedef bit<15> switch_tunnel_ip_index_t;
typedef bit<16> switch_tunnel_nexthop_t;
typedef bit<24> switch_tunnel_vni_t;
struct switch_tunnel_metadata_t {
    switch_tunnel_type_t         type;
    switch_tunnel_mode_t         ecn_mode;
    switch_tunnel_index_t        index;
    switch_tunnel_mapper_index_t mapper_index;
    switch_tunnel_ip_index_t     dip_index;
    switch_tunnel_vni_t          vni;
    switch_tunnel_mode_t         qos_mode;
    switch_tunnel_mode_t         ttl_mode;
    bit<8>                       decap_ttl;
    bit<8>                       decap_tos;
    bit<3>                       decap_exp;
    bit<16>                      hash;
    bool                         terminate;
    bit<8>                       nvgre_flow_id;
    bit<2>                       mpls_pop_count;
    bit<3>                       mpls_push_count;
    bit<8>                       mpls_encap_ttl;
    bit<3>                       mpls_encap_exp;
    bit<1>                       mpls_swap;
    bit<128>                     srh_next_sid;
    bit<8>                       srh_next_hdr;
    bit<3>                       srv6_seg_len;
    bit<6>                       srh_hdr_len;
    bool                         remove_srh;
    bool                         pop_active_segment;
    bool                         srh_decap_forward;
}

struct switch_nvgre_value_set_t {
    bit<32> vsid_flowid;
}

typedef bit<8> switch_dtel_report_type_t;
typedef bit<8> switch_ifa_sample_id_t;
typedef bit<4> switch_ingress_nat_hit_type_t;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_NONE = 0;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_FLOW_NONE = 1;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_FLOW_NAPT = 2;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_FLOW_NAT = 3;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_DEST_NONE = 4;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_DEST_NAPT = 5;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_DEST_NAT = 6;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_SRC_NONE = 7;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_SRC_NAPT = 8;
const switch_ingress_nat_hit_type_t SWITCH_NAT_HIT_TYPE_SRC_NAT = 9;
typedef bit<1> switch_nat_zone_t;
const switch_nat_zone_t SWITCH_NAT_INSIDE_ZONE_ID = 0;
const switch_nat_zone_t SWITCH_NAT_OUTSIDE_ZONE_ID = 1;
struct switch_nat_ingress_metadata_t {
    switch_ingress_nat_hit_type_t hit;
    switch_nat_zone_t             ingress_zone;
    bit<16>                       dnapt_index;
    bit<16>                       snapt_index;
    bool                          nat_disable;
    bool                          dnat_pool_hit;
}

enum bit<2> LinkToType {
    Unknown = 0,
    Switch = 1,
    Server = 2
}

const bit<32> msb_set_32b = 32w0x80000000;
@pa_mutually_exclusive("ingress" , "lkp.arp_opcode" , "lkp.ip_src_addr") @pa_mutually_exclusive("ingress" , "lkp.arp_opcode" , "lkp.ip_dst_addr") @pa_mutually_exclusive("ingress" , "lkp.arp_opcode" , "lkp.ipv6_flow_label") @pa_mutually_exclusive("ingress" , "lkp.arp_opcode" , "lkp.ip_proto") @pa_mutually_exclusive("ingress" , "lkp.arp_opcode" , "lkp.ip_ttl") @pa_mutually_exclusive("ingress" , "lkp.arp_opcode" , "lkp.ip_tos") struct switch_flags_t {
    bool                   ipv4_checksum_err;
    bool                   inner_ipv4_checksum_err;
    bool                   inner2_ipv4_checksum_err;
    bool                   link_local;
    bool                   routed;
    bool                   l2_tunnel_encap;
    bool                   acl_deny;
    bool                   racl_deny;
    bool                   port_vlan_miss;
    bool                   rmac_hit;
    bool                   dmac_miss;
    switch_myip_type_t     myip;
    bool                   glean;
    bool                   storm_control_drop;
    bool                   port_meter_drop;
    bool                   flood_to_multicast_routers;
    bool                   peer_link;
    bool                   capture_ts;
    bool                   mac_pkt_class;
    bool                   pfc_wd_drop;
    bool                   bypass_egress;
    bool                   mpls_trap;
    bool                   srv6_trap;
    bool                   wred_drop;
    bool                   port_isolation_packet_drop;
    bool                   bport_isolation_packet_drop;
    bool                   fib_lpm_miss;
    bool                   fib_drop;
    switch_packet_action_t meter_packet_action;
    switch_packet_action_t vrf_ttl_violation;
    bool                   vrf_ttl_violation_valid;
    bool                   vlan_arp_suppress;
    switch_packet_action_t vrf_ip_options_violation;
    bool                   vrf_unknown_l3_multicast_trap;
    bool                   bfd_to_cpu;
    bool                   redirect_to_cpu;
    bool                   copy_cancel;
    bool                   to_cpu;
}

struct switch_checks_t {
    switch_port_lag_index_t same_if;
    bool                    mrpf;
    bool                    urpf;
    switch_nat_zone_t       same_zone_check;
    switch_bd_t             same_bd;
    switch_mtu_t            mtu;
    bool                    stp;
}

struct switch_ip_metadata_t {
    bool unicast_enable;
    bool multicast_enable;
    bool multicast_snooping;
}

struct switch_lookup_fields_t {
    switch_pkt_type_t pkt_type;
    mac_addr_t        mac_src_addr;
    mac_addr_t        mac_dst_addr;
    bit<16>           mac_type;
    bit<3>            pcp;
    bit<1>            dei;
    bit<16>           arp_opcode;
    switch_ip_type_t  ip_type;
    bit<8>            ip_proto;
    bit<8>            ip_ttl;
    bit<8>            ip_tos;
    switch_ip_frag_t  ip_frag;
    bit<128>          ip_src_addr;
    bit<128>          ip_dst_addr;
    bit<32>           ip_src_addr_95_64;
    bit<32>           ip_dst_addr_95_64;
    bit<20>           ipv6_flow_label;
    bit<8>            tcp_flags;
    bit<16>           l4_src_port;
    bit<16>           l4_dst_port;
    bit<8>            inner_tcp_flags;
    bit<16>           inner_l4_src_port;
    bit<16>           inner_l4_dst_port;
    bit<16>           hash_l4_src_port;
    bit<16>           hash_l4_dst_port;
    bool              mpls_pkt;
    bit<1>            mpls_router_alert_label;
    bit<20>           mpls_lookup_label;
}

struct switch_hash_fields_t {
    mac_addr_t       mac_src_addr;
    mac_addr_t       mac_dst_addr;
    bit<16>          mac_type;
    switch_ip_type_t ip_type;
    bit<8>           ip_proto;
    bit<128>         ip_src_addr;
    bit<128>         ip_dst_addr;
    bit<16>          l4_src_port;
    bit<16>          l4_dst_port;
    bit<20>          ipv6_flow_label;
    bit<32>          outer_ip_src_addr;
    bit<32>          outer_ip_dst_addr;
}

@flexible struct switch_bridged_metadata_base_t {
    switch_port_t           ingress_port;
    switch_port_lag_index_t ingress_port_lag_index;
    switch_bd_t             ingress_bd;
    switch_nexthop_t        nexthop;
    switch_pkt_type_t       pkt_type;
    bool                    routed;
    bool                    bypass_egress;
    switch_cpu_reason_t     cpu_reason;
    bit<32>                 timestamp;
    switch_tc_t             tc;
    switch_qid_t            qid;
    switch_pkt_color_t      color;
    switch_vrf_t            vrf;
}

@flexible struct switch_bridged_metadata_mirror_extension_t {
    switch_pkt_src_t         src;
    switch_mirror_type_t     type;
    switch_mirror_session_t  session_id;
    switch_mirror_meter_id_t meter_index;
}

@flexible struct switch_bridged_metadata_acl_extension_t {
    bit<16> l4_src_port;
    bit<16> l4_dst_port;
    bit<8>  tcp_flags;
}

@flexible struct switch_bridged_metadata_tunnel_extension_t {
    switch_tunnel_nexthop_t tunnel_nexthop;
    bit<16>                 hash;
    switch_tunnel_mode_t    ttl_mode;
    switch_tunnel_mode_t    qos_mode;
    bool                    terminate;
}

@pa_mutually_exclusive("egress" , "hdr.erspan" , "hdr.vxlan") @pa_mutually_exclusive("egress" , "hdr.gre" , "hdr.vxlan") @pa_mutually_exclusive("egress" , "hdr.erspan_type3" , "hdr.vxlan") @pa_mutually_exclusive("egress" , "hdr.erspan_type2" , "hdr.vxlan") @pa_mutually_exclusive("egress" , "hdr.erspan_type3" , "hdr.erspan_type2") @flexible header switch_fp_bridged_metadata_h {
    bit<16>                 ingress_bd_tt_myip;
    switch_pkt_type_t       pkt_type;
    bool                    routed;
    bit<32>                 timestamp;
    switch_tc_t             tc;
    bit<8>                  qid_icos;
    switch_pkt_color_t      color;
    bool                    ipv4_checksum_err;
    bool                    rmac_hit;
    bool                    fib_drop;
    bool                    fib_lpm_miss;
    bit<1>                  multicast_hit;
    bool                    acl_deny;
    bool                    copy_cancel;
    bool                    nat_disable;
    switch_drop_reason_t    drop_reason;
    switch_hostif_trap_t    hostif_trap_id;
    switch_mirror_session_t mirror_session_id;
    switch_fwd_type_t       fwd_type;
    switch_fwd_idx_t        fwd_idx;
}

typedef bit<8> switch_bridge_type_t;
header switch_bridged_metadata_h {
    switch_pkt_src_t                           src;
    switch_bridge_type_t                       type;
    switch_bridged_metadata_base_t             base;
    switch_bridged_metadata_acl_extension_t    acl;
    switch_bridged_metadata_tunnel_extension_t tunnel;
    switch_mirror_metadata_t                   mirror;
}

struct switch_port_metadata_t {
    switch_port_lag_index_t    port_lag_index;
    switch_ig_port_lag_label_t port_lag_label;
}

struct switch_fp_port_metadata_t {
    bit<1> unused;
}

typedef bit<2> switch_cons_hash_ip_seq_t;
const switch_cons_hash_ip_seq_t SWITCH_CONS_HASH_IP_SEQ_NONE = 0;
const switch_cons_hash_ip_seq_t SWITCH_CONS_HASH_IP_SEQ_SIPDIP = 1;
const switch_cons_hash_ip_seq_t SWITCH_CONS_HASH_IP_SEQ_DIPSIP = 2;
@pa_auto_init_metadata @pa_container_size("ingress" , "local_md.mirror.src" , 8) @pa_container_size("ingress" , "local_md.mirror.type" , 8) @pa_alias("ingress" , "local_md.egress_port" , "ig_intr_md_for_tm.ucast_egress_port") @pa_alias("ingress" , "local_md.multicast.id" , "ig_intr_md_for_tm.mcast_grp_b") @pa_alias("ingress" , "local_md.qos.qid" , "ig_intr_md_for_tm.qid") @pa_alias("ingress" , "local_md.qos.icos" , "ig_intr_md_for_tm.ingress_cos") @pa_alias("ingress" , "ig_intr_md_for_dprsr.mirror_type" , "local_md.mirror.type") @pa_container_size("ingress" , "local_md.nat.snapt_index" , 16) @pa_container_size("egress" , "local_md.mirror.src" , 8) @pa_container_size("egress" , "local_md.mirror.type" , 8) struct switch_local_metadata_t {
    switch_port_t                 ingress_port;
    switch_port_t                 egress_port;
    switch_port_lag_index_t       ingress_port_lag_index;
    switch_port_lag_index_t       egress_port_lag_index;
    switch_bd_t                   bd;
    switch_bd_t                   ingress_outer_bd;
    switch_bd_t                   ingress_bd;
    switch_vrf_t                  vrf;
    switch_nexthop_t              nexthop;
    switch_tunnel_nexthop_t       tunnel_nexthop;
    switch_nexthop_t              acl_nexthop;
    bool                          acl_port_redirect;
    switch_nexthop_t              unused_nexthop;
    bit<32>                       timestamp;
    switch_ecmp_hash_t            ecmp_hash;
    switch_ecmp_hash_t            outer_ecmp_hash;
    switch_hash_t                 lag_hash;
    switch_flags_t                flags;
    switch_checks_t               checks;
    switch_ingress_bypass_t       bypass;
    switch_ip_metadata_t          ipv4;
    switch_ip_metadata_t          ipv6;
    switch_ig_port_lag_label_t    ingress_port_lag_label;
    switch_bd_label_t             bd_label;
    switch_l4_port_label_t        l4_src_port_label;
    switch_l4_port_label_t        l4_dst_port_label;
    switch_etype_label_t          etype_label;
    switch_mac_addr_label_t       qos_mac_label;
    switch_mac_addr_label_t       pbr_mac_label;
    switch_mac_addr_label_t       mirror_mac_label;
    switch_drop_reason_t          l2_drop_reason;
    switch_drop_reason_t          drop_reason;
    switch_cpu_reason_t           cpu_reason;
    switch_lookup_fields_t        lkp;
    switch_hostif_trap_t          hostif_trap_id;
    switch_hash_fields_t          hash_fields;
    switch_multicast_metadata_t   multicast;
    switch_stp_metadata_t         stp;
    switch_qos_metadata_t         qos;
    switch_sflow_metadata_t       sflow;
    switch_tunnel_metadata_t      tunnel;
    switch_learning_metadata_t    learning;
    switch_mirror_metadata_t      mirror;
    mac_addr_t                    same_mac;
    switch_user_metadata_t        user_metadata;
    bit<16>                       tcp_udp_checksum;
    switch_nat_ingress_metadata_t nat;
    bit<10>                       partition_key;
    bit<12>                       partition_index;
    switch_fib_label_t            fib_label;
    switch_cons_hash_ip_seq_t     cons_hash_v6_ip_seq;
    switch_pkt_src_t              pkt_src;
    switch_pkt_length_t           pkt_length;
    bit<32>                       egress_timestamp;
    bit<32>                       ingress_timestamp;
    switch_eg_port_lag_label_t    egress_port_lag_label;
    switch_nexthop_type_t         nexthop_type;
    bool                          inner_ipv4_checksum_update_en;
    switch_isolation_group_t      port_isolation_group;
    switch_isolation_group_t      bport_isolation_group;
    switch_ports_group_label_t    in_ports_group_label_mirror;
    switch_ports_group_label_t    out_ports_group_label_ipv4;
    switch_ports_group_label_t    out_ports_group_label_ipv6;
}

struct switch_mirror_metadata_h {
    switch_port_mirror_metadata_h port;
    switch_cpu_mirror_metadata_h  cpu;
}

struct switch_header_t {
    switch_bridged_metadata_h    bridged_md;
    switch_fp_bridged_metadata_h fp_bridged_md;
    ethernet_h                   ethernet;
    fabric_h                     fabric;
    cpu_h                        cpu;
    timestamp_h                  timestamp;
    vlan_tag_h[2]                vlan_tag;
    mpls_h[3]                    mpls;
    ipv4_h                       ipv4;
    ipv4_option_h                ipv4_option;
    ipv6_h                       ipv6;
    arp_h                        arp;
    ipv6_srh_h                   srh_base;
    ipv6_segment_h[2]            srh_seg_list;
    udp_h                        udp;
    icmp_h                       icmp;
    igmp_h                       igmp;
    tcp_h                        tcp;
    dtel_report_v05_h            dtel;
    dtel_report_base_h           dtel_report;
    dtel_switch_local_report_h   dtel_switch_local_report;
    dtel_drop_report_h           dtel_drop_report;
    rocev2_bth_h                 rocev2_bth;
    gtpu_h                       gtp;
    vxlan_h                      vxlan;
    gre_h                        gre;
    nvgre_h                      nvgre;
    geneve_h                     geneve;
    erspan_h                     erspan;
    erspan_type2_h               erspan_type2;
    erspan_type3_h               erspan_type3;
    erspan_platform_h            erspan_platform;
    ethernet_h                   inner_ethernet;
    ipv4_h                       inner_ipv4;
    ipv6_h                       inner_ipv6;
    udp_h                        inner_udp;
    tcp_h                        inner_tcp;
    icmp_h                       inner_icmp;
    igmp_h                       inner_igmp;
    ipv4_h                       inner2_ipv4;
    ipv6_h                       inner2_ipv6;
    udp_h                        inner2_udp;
    tcp_h                        inner2_tcp;
    icmp_h                       inner2_icmp;
    pad_h                        pad;
}

action add_bridged_md(inout switch_bridged_metadata_h bridged_md, in switch_local_metadata_t local_md) {
    bridged_md.setValid();
    bridged_md.src = SWITCH_PKT_SRC_BRIDGED;
    bridged_md.type = 0;
    bridged_md.base.ingress_port = local_md.ingress_port;
    bridged_md.base.ingress_port_lag_index = local_md.ingress_port_lag_index;
    bridged_md.base.ingress_bd = local_md.bd;
    bridged_md.base.nexthop = local_md.nexthop;
    bridged_md.base.pkt_type = local_md.lkp.pkt_type;
    bridged_md.base.routed = local_md.flags.routed;
    bridged_md.base.bypass_egress = local_md.flags.bypass_egress;
    bridged_md.base.cpu_reason = local_md.cpu_reason;
    bridged_md.base.timestamp = local_md.timestamp;
    bridged_md.base.tc = local_md.qos.tc;
    bridged_md.base.qid = local_md.qos.qid;
    bridged_md.base.color = local_md.qos.color;
    bridged_md.base.vrf = local_md.vrf;
    bridged_md.acl.l4_src_port = local_md.lkp.l4_src_port;
    bridged_md.acl.l4_dst_port = local_md.lkp.l4_dst_port;
    bridged_md.acl.tcp_flags = local_md.lkp.tcp_flags;
    bridged_md.tunnel.tunnel_nexthop = local_md.tunnel_nexthop;
    bridged_md.tunnel.hash = local_md.lag_hash[15:0];
    bridged_md.tunnel.ttl_mode = local_md.tunnel.ttl_mode;
    bridged_md.tunnel.qos_mode = local_md.tunnel.qos_mode;
    bridged_md.tunnel.terminate = local_md.tunnel.terminate;
}
action set_ig_intr_md(in switch_local_metadata_t local_md, inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr, inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm) {
    ig_intr_md_for_tm.mcast_grp_b = local_md.multicast.id;
    ig_intr_md_for_tm.level2_mcast_hash = local_md.lag_hash[28:16];
    ig_intr_md_for_tm.rid = local_md.bd;
    ig_intr_md_for_tm.qid = local_md.qos.qid;
    ig_intr_md_for_tm.ingress_cos = local_md.qos.icos;
}
action copy_mirror_to_bridged_md(inout switch_bridged_metadata_h bridged_md, inout switch_local_metadata_t local_md) {
    bridged_md.mirror.type = local_md.mirror.type;
    bridged_md.mirror.src = local_md.mirror.src;
    bridged_md.mirror.session_id = local_md.mirror.session_id;
    bridged_md.mirror.meter_index = local_md.mirror.meter_index;
    local_md.mirror.type = 0;
    local_md.mirror.src = SWITCH_PKT_SRC_BRIDGED;
    local_md.mirror.session_id = 0;
    local_md.mirror.meter_index = 0;
}
control SetEgIntrMd(inout switch_header_t hdr, in switch_local_metadata_t local_md, inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr, inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
    apply {
        if (local_md.mirror.type != 0) {
            eg_intr_md_for_dprsr.mirror_type = (bit<4>)local_md.mirror.type;
            if (local_md.mirror.src == SWITCH_PKT_SRC_CLONED_EGRESS_IN_PKT) {
                eg_intr_md_for_dprsr.mirror_io_select = 0;
            } else {
                eg_intr_md_for_dprsr.mirror_io_select = 1;
            }
        }
    }
}

control EnableFragHash(inout switch_lookup_fields_t lkp) {
    apply {
        if (lkp.ip_frag != SWITCH_IP_FRAG_NON_FRAG) {
            lkp.hash_l4_dst_port = 0;
            lkp.hash_l4_src_port = 0;
        } else {
            lkp.hash_l4_dst_port = lkp.l4_dst_port;
            lkp.hash_l4_src_port = lkp.l4_src_port;
        }
    }
}

control Ipv4Hash(in switch_lookup_fields_t lkp, out switch_ecmp_hash_t hash) {
    @name(".ipv4_hash") Hash<switch_ecmp_hash_t>(HashAlgorithm_t.CRC16) ipv4_hash;
    bit<32> ip_src_addr = lkp.ip_src_addr[95:64];
    bit<32> ip_dst_addr = lkp.ip_dst_addr[95:64];
    bit<8> ip_proto = lkp.ip_proto;
    bit<16> l4_dst_port = lkp.hash_l4_dst_port;
    bit<16> l4_src_port = lkp.hash_l4_src_port;
    apply {
        hash = ipv4_hash.get({ ip_src_addr, ip_dst_addr, ip_proto, l4_dst_port, l4_src_port });
    }
}

control Ipv6Hash(in switch_lookup_fields_t lkp, out switch_ecmp_hash_t hash) {
    @name(".ipv6_hash") Hash<switch_ecmp_hash_t>(HashAlgorithm_t.CRC16) ipv6_hash;
    bit<128> ip_src_addr = lkp.ip_src_addr;
    bit<128> ip_dst_addr = lkp.ip_dst_addr;
    bit<8> ip_proto = lkp.ip_proto;
    bit<16> l4_dst_port = lkp.hash_l4_dst_port;
    bit<16> l4_src_port = lkp.hash_l4_src_port;
    bit<20> ipv6_flow_label = lkp.ipv6_flow_label;
    apply {
        hash = ipv6_hash.get({ ipv6_flow_label, ip_src_addr, ip_dst_addr, ip_proto, l4_dst_port, l4_src_port });
    }
}

control OuterIpv4Hash(in switch_lookup_fields_t lkp, out switch_ecmp_hash_t hash) {
    @name(".outer_ipv4_hash") Hash<switch_ecmp_hash_t>(HashAlgorithm_t.CRC16) ipv4_hash;
    bit<32> ip_src_addr = lkp.ip_src_addr[95:64];
    bit<32> ip_dst_addr = lkp.ip_dst_addr[95:64];
    bit<8> ip_proto = lkp.ip_proto;
    bit<16> l4_dst_port = lkp.hash_l4_dst_port;
    bit<16> l4_src_port = lkp.hash_l4_src_port;
    apply {
        hash = ipv4_hash.get({ ip_proto, l4_dst_port, l4_src_port, ip_dst_addr, ip_src_addr });
    }
}

control OuterIpv6Hash(in switch_lookup_fields_t lkp, out switch_ecmp_hash_t hash) {
    @name(".outer_ipv6_hash") Hash<switch_ecmp_hash_t>(HashAlgorithm_t.CRC16) ipv6_hash;
    bit<128> ip_src_addr = lkp.ip_src_addr;
    bit<128> ip_dst_addr = lkp.ip_dst_addr;
    bit<8> ip_proto = lkp.ip_proto;
    bit<16> l4_dst_port = lkp.hash_l4_dst_port;
    bit<16> l4_src_port = lkp.hash_l4_src_port;
    bit<20> ipv6_flow_label = lkp.ipv6_flow_label;
    apply {
        hash = ipv6_hash.get({ ip_proto, l4_dst_port, l4_src_port, ipv6_flow_label, ip_dst_addr, ip_src_addr });
    }
}

control NonIpHash(in switch_header_t hdr, in switch_local_metadata_t local_md, out switch_hash_t hash) {
    @name(".non_ip_hash") Hash<switch_hash_t>(HashAlgorithm_t.CRC32) non_ip_hash;
    mac_addr_t mac_dst_addr = hdr.ethernet.dst_addr;
    mac_addr_t mac_src_addr = hdr.ethernet.src_addr;
    bit<16> mac_type = hdr.ethernet.ether_type;
    switch_port_t port = local_md.ingress_port;
    apply {
        hash = non_ip_hash.get({ port, mac_type, mac_src_addr, mac_dst_addr });
    }
}

control Lagv4Hash(in switch_lookup_fields_t lkp, out switch_hash_t hash) {
    @name(".lag_v4_hash") Hash<switch_hash_t>(HashAlgorithm_t.CRC32) lag_v4_hash;
    bit<32> ip_src_addr = lkp.ip_src_addr[95:64];
    bit<32> ip_dst_addr = lkp.ip_dst_addr[95:64];
    bit<8> ip_proto = lkp.ip_proto;
    bit<16> l4_dst_port = lkp.hash_l4_dst_port;
    bit<16> l4_src_port = lkp.hash_l4_src_port;
    apply {
        hash = lag_v4_hash.get({ ip_src_addr, ip_dst_addr, ip_proto, l4_dst_port, l4_src_port });
    }
}

control Lagv6Hash(in switch_lookup_fields_t lkp, out switch_hash_t hash) {
    @name(".lag_v6_hash") Hash<switch_hash_t>(HashAlgorithm_t.CRC32) lag_v6_hash;
    bit<128> ip_src_addr = lkp.ip_src_addr;
    bit<128> ip_dst_addr = lkp.ip_dst_addr;
    bit<8> ip_proto = lkp.ip_proto;
    bit<16> l4_dst_port = lkp.hash_l4_dst_port;
    bit<16> l4_src_port = lkp.hash_l4_src_port;
    bit<20> ipv6_flow_label = lkp.ipv6_flow_label;
    apply {
        hash = lag_v6_hash.get({ ipv6_flow_label, ip_src_addr, ip_dst_addr, ip_proto, l4_dst_port, l4_src_port });
    }
}

control V4ConsistentHash(in bit<32> sip, in bit<32> dip, in bit<16> low_l4_port, in bit<16> high_l4_port, in bit<8> protocol, inout switch_ecmp_hash_t hash) {
    Hash<switch_ecmp_hash_t>(HashAlgorithm_t.CRC16) ipv4_inner_hash;
    bit<32> high_ip;
    bit<32> low_ip;
    action step_v4() {
        high_ip = max(sip, dip);
        low_ip = min(sip, dip);
    }
    action v4_calc_hash() {
        hash = ipv4_inner_hash.get({ low_ip, high_ip, protocol, low_l4_port, high_l4_port });
    }
    apply {
        step_v4();
        v4_calc_hash();
    }
}

control V6ConsistentHash64bIpSeq(in bit<64> sip, in bit<64> dip, inout switch_cons_hash_ip_seq_t ip_seq) {
    bit<32> high_63_32_ip;
    bit<32> src_63_32_ip;
    bit<32> high_31_0_ip;
    bit<32> src_31_0_ip;
    action step_63_32_v6() {
        high_63_32_ip = max(sip[63:32], dip[63:32]);
        src_63_32_ip = sip[63:32];
    }
    action step_31_0_v6() {
        high_31_0_ip = max(sip[31:0], dip[31:0]);
        src_31_0_ip = sip[31:0];
    }
    apply {
        ip_seq = SWITCH_CONS_HASH_IP_SEQ_SIPDIP;
        step_63_32_v6();
        step_31_0_v6();
        if (sip[63:32] != dip[63:32]) {
            if (high_63_32_ip == src_63_32_ip) {
                ip_seq = SWITCH_CONS_HASH_IP_SEQ_DIPSIP;
            }
        } else if (sip[31:0] != dip[31:0]) {
            if (high_31_0_ip == src_31_0_ip) {
                ip_seq = SWITCH_CONS_HASH_IP_SEQ_DIPSIP;
            }
        } else {
            ip_seq = SWITCH_CONS_HASH_IP_SEQ_NONE;
        }
    }
}

control V6ConsistentHash(in bit<128> sip, in bit<128> dip, in switch_cons_hash_ip_seq_t ip_seq, in bit<16> low_l4_port, in bit<16> high_l4_port, in bit<8> protocol, inout switch_ecmp_hash_t hash) {
    bit<32> high_31_0_ip;
    bit<32> low_31_0_ip;
    bit<32> src_31_0_ip;
    Hash<switch_ecmp_hash_t>(HashAlgorithm_t.CRC16) ipv6_inner_hash_1;
    Hash<switch_ecmp_hash_t>(HashAlgorithm_t.CRC16) ipv6_inner_hash_2;
    bit<32> high_63_32_ip;
    bit<32> src_63_32_ip;
    action step_63_32_v6() {
        high_63_32_ip = max(sip[63:32], dip[63:32]);
        src_63_32_ip = sip[63:32];
    }
    action step_31_0_v6() {
        high_31_0_ip = max(sip[31:0], dip[31:0]);
        src_31_0_ip = sip[31:0];
    }
    action v6_calc_hash_sip_dip() {
        hash = ipv6_inner_hash_1.get({ sip, dip, protocol, low_l4_port, high_l4_port });
    }
    action v6_calc_hash_dip_sip() {
        hash = ipv6_inner_hash_2.get({ dip, sip, protocol, low_l4_port, high_l4_port });
    }
    apply {
        if (ip_seq == SWITCH_CONS_HASH_IP_SEQ_SIPDIP) {
            v6_calc_hash_sip_dip();
        } else if (ip_seq == SWITCH_CONS_HASH_IP_SEQ_DIPSIP) {
            v6_calc_hash_dip_sip();
        } else {
            step_63_32_v6();
            step_31_0_v6();
            if (sip[63:32] != dip[63:32]) {
                if (high_63_32_ip == src_63_32_ip) {
                    v6_calc_hash_dip_sip();
                } else {
                    v6_calc_hash_sip_dip();
                }
            } else if (high_31_0_ip == src_31_0_ip) {
                v6_calc_hash_dip_sip();
            } else {
                v6_calc_hash_sip_dip();
            }
        }
    }
}

control StartConsistentInnerHash(in switch_header_t hd, inout switch_local_metadata_t ig_m) {
    V6ConsistentHash64bIpSeq() compute_v6_cons_hash_64bit_ipseq;
    apply {
        if (hd.inner_ipv6.isValid() && ig_m.tunnel.type != SWITCH_INGRESS_TUNNEL_TYPE_NVGRE_ST) {
            compute_v6_cons_hash_64bit_ipseq.apply(hd.inner_ipv6.src_addr[127:64], hd.inner_ipv6.dst_addr[127:64], ig_m.cons_hash_v6_ip_seq);
        }
    }
}

control ConsistentInnerHash(in switch_header_t hd, inout switch_local_metadata_t ig_m) {
    bit<32> high_31_0_ip;
    bit<32> low_31_0_ip;
    Hash<switch_ecmp_hash_t>(HashAlgorithm_t.CRC16) ipv6_inner_hash;
    Hash<bit<16>>(HashAlgorithm_t.IDENTITY) l4_tcp_src_p_hash;
    Hash<bit<16>>(HashAlgorithm_t.IDENTITY) l4_udp_src_p_hash;
    bit<16> l4_src_port;
    bit<16> low_l4_port = 0;
    bit<16> high_l4_port = 0;
    action step_tcp_src_port() {
        l4_src_port = l4_tcp_src_p_hash.get({ 16w0 ++ +hd.inner_tcp.src_port });
    }
    action step_udp_src_port() {
        l4_src_port = l4_udp_src_p_hash.get({ 16w0 ++ +hd.inner_udp.src_port });
    }
    action step_tcp_l4_port() {
        high_l4_port = (bit<16>)max(l4_src_port, hd.inner_tcp.dst_port);
        low_l4_port = (bit<16>)min(l4_src_port, hd.inner_tcp.dst_port);
    }
    action step_udp_l4_port() {
        high_l4_port = (bit<16>)max(l4_src_port, hd.inner_udp.dst_port);
        low_l4_port = (bit<16>)min(l4_src_port, hd.inner_udp.dst_port);
    }
    action step_31_0_v6() {
        high_31_0_ip = max(hd.inner_ipv6.src_addr[31:0], hd.inner_ipv6.dst_addr[31:0]);
        low_31_0_ip = min(hd.inner_ipv6.src_addr[31:0], hd.inner_ipv6.dst_addr[31:0]);
    }
    action v6_calc_31_0_hash() {
        ig_m.ecmp_hash = ipv6_inner_hash.get({ low_31_0_ip, high_31_0_ip, hd.inner_ipv6.next_hdr, low_l4_port, high_l4_port });
    }
    V6ConsistentHash() compute_v6_cons_hash;
    V4ConsistentHash() compute_v4_cons_hash;
    apply {
        if (hd.inner_udp.isValid()) {
            step_udp_src_port();
            step_udp_l4_port();
        } else if (hd.inner_tcp.isValid()) {
            step_tcp_src_port();
            step_tcp_l4_port();
        }
        if (hd.inner_ipv6.isValid()) {
            if (ig_m.tunnel.type != SWITCH_INGRESS_TUNNEL_TYPE_NVGRE_ST) {
                compute_v6_cons_hash.apply(hd.inner_ipv6.src_addr, hd.inner_ipv6.dst_addr, ig_m.cons_hash_v6_ip_seq, low_l4_port, high_l4_port, hd.inner_ipv6.next_hdr, ig_m.ecmp_hash);
            } else {
                step_31_0_v6();
                v6_calc_31_0_hash();
            }
        } else if (hd.inner_ipv4.isValid()) {
            compute_v4_cons_hash.apply(hd.inner_ipv4.src_addr, hd.inner_ipv4.dst_addr, low_l4_port, high_l4_port, hd.inner_ipv4.protocol, ig_m.ecmp_hash);
        }
    }
}

control InnerDtelv4Hash(in switch_header_t hdr, in switch_local_metadata_t local_md, out switch_hash_t hash) {
    @name(".inner_dtelv4_hash") Hash<switch_hash_t>(HashAlgorithm_t.CRC32) inner_dtelv4_hash;
    bit<32> ip_src_addr = hdr.inner_ipv4.src_addr;
    bit<32> ip_dst_addr = hdr.inner_ipv4.dst_addr;
    bit<8> ip_proto = hdr.inner_ipv4.protocol;
    bit<16> l4_src_port = hdr.udp.src_port;
    apply {
        hash = inner_dtelv4_hash.get({ ip_src_addr, ip_dst_addr, ip_proto, l4_src_port });
    }
}

control InnerDtelv6Hash(in switch_header_t hdr, in switch_local_metadata_t local_md, out switch_hash_t hash) {
    @name(".inner_dtelv6_hash") Hash<switch_hash_t>(HashAlgorithm_t.CRC32) inner_dtelv6_hash;
    bit<128> ip_src_addr = hdr.ipv6.src_addr;
    bit<128> ip_dst_addr = hdr.ipv6.dst_addr;
    bit<8> ip_proto = hdr.ipv6.next_hdr;
    bit<16> l4_src_port = hdr.udp.src_port;
    bit<20> ipv6_flow_label = hdr.inner_ipv6.flow_label;
    apply {
        hash = inner_dtelv6_hash.get({ ipv6_flow_label, ip_src_addr, ip_dst_addr, ip_proto, l4_src_port });
    }
}

control RotateHash(inout switch_local_metadata_t local_md) {
    @name(".rotate_by_0") action rotate_by_0() {
    }
    @name(".rotate_by_1") action rotate_by_1() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[1 - 1:0] ++ local_md.ecmp_hash[15:1];
    }
    @name(".rotate_by_2") action rotate_by_2() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[2 - 1:0] ++ local_md.ecmp_hash[15:2];
    }
    @name(".rotate_by_3") action rotate_by_3() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[3 - 1:0] ++ local_md.ecmp_hash[15:3];
    }
    @name(".rotate_by_4") action rotate_by_4() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[4 - 1:0] ++ local_md.ecmp_hash[15:4];
    }
    @name(".rotate_by_5") action rotate_by_5() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[5 - 1:0] ++ local_md.ecmp_hash[15:5];
    }
    @name(".rotate_by_6") action rotate_by_6() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[6 - 1:0] ++ local_md.ecmp_hash[15:6];
    }
    @name(".rotate_by_7") action rotate_by_7() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[7 - 1:0] ++ local_md.ecmp_hash[15:7];
    }
    @name(".rotate_by_8") action rotate_by_8() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[8 - 1:0] ++ local_md.ecmp_hash[15:8];
    }
    @name(".rotate_by_9") action rotate_by_9() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[9 - 1:0] ++ local_md.ecmp_hash[15:9];
    }
    @name(".rotate_by_10") action rotate_by_10() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[10 - 1:0] ++ local_md.ecmp_hash[15:10];
    }
    @name(".rotate_by_11") action rotate_by_11() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[11 - 1:0] ++ local_md.ecmp_hash[15:11];
    }
    @name(".rotate_by_12") action rotate_by_12() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[12 - 1:0] ++ local_md.ecmp_hash[15:12];
    }
    @name(".rotate_by_13") action rotate_by_13() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[13 - 1:0] ++ local_md.ecmp_hash[15:13];
    }
    @name(".rotate_by_14") action rotate_by_14() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[14 - 1:0] ++ local_md.ecmp_hash[15:14];
    }
    @name(".rotate_by_15") action rotate_by_15() {
        local_md.ecmp_hash[15:0] = local_md.ecmp_hash[15 - 1:0] ++ local_md.ecmp_hash[15:15];
    }
    @name(".rotate_hash") table rotate_hash {
        actions = {
            rotate_by_0;
            rotate_by_1;
            rotate_by_2;
            rotate_by_3;
            rotate_by_4;
            rotate_by_5;
            rotate_by_6;
            rotate_by_7;
            rotate_by_8;
            rotate_by_9;
            rotate_by_10;
            rotate_by_11;
            rotate_by_12;
            rotate_by_13;
            rotate_by_14;
            rotate_by_15;
        }
        size = 16;
        default_action = rotate_by_0;
    }
    apply {
        rotate_hash.apply();
    }
}

control PreIngressAcl(in switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=512) {
    bool is_acl_enabled;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".set_acl_status") action set_acl_status(bool enabled) {
        is_acl_enabled = enabled;
    }
    @name(".device_to_acl") table device_to_acl {
        actions = {
            set_acl_status;
        }
        default_action = set_acl_status(false);
        size = 1;
    }
    @name(".pre_ingress_acl_no_action") action no_action() {
        stats.count();
    }
    @name(".pre_ingress_acl_drop") action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }
    @name(".pre_ingress_acl_set_vrf") action set_vrf(switch_vrf_t vrf) {
        local_md.vrf = vrf;
        stats.count();
    }
    @name(".pre_ingress_acl") table acl {
        key = {
            hdr.ethernet.src_addr   : ternary;
            hdr.ethernet.dst_addr   : ternary;
            local_md.lkp.mac_type   : ternary;
            local_md.lkp.ip_src_addr: ternary;
            local_md.lkp.ip_dst_addr: ternary;
            local_md.lkp.ip_tos     : ternary;
            local_md.ingress_port   : ternary;
        }
        actions = {
            set_vrf;
            drop;
            no_action;
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        device_to_acl.apply();
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_ACL != 0) && is_acl_enabled == true) {
            acl.apply();
        }
    }
}

control IngressIpAcl(inout switch_local_metadata_t local_md, out switch_nexthop_t acl_nexthop)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action set_tc(switch_tc_t tc) {
        local_md.qos.tc = tc;
        stats.count();
    }
    action set_color(switch_pkt_color_t color) {
        local_md.qos.color = color;
        stats.count();
    }
    action redirect_port(switch_user_metadata_t user_metadata, switch_port_lag_index_t egress_port_lag_index) {
        local_md.flags.acl_deny = false;
        local_md.egress_port_lag_index = egress_port_lag_index;
        local_md.acl_port_redirect = true;
        local_md.user_metadata = user_metadata;
        stats.count();
    }
    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_INGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    action no_action() {
        stats.count();
    }
    action permit(switch_user_metadata_t user_metadata, switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.flags.acl_deny = false;
        local_md.user_metadata = user_metadata;
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }
    action copy_to_cpu(switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action trap(switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        drop();
    }
    action redirect_nexthop(switch_user_metadata_t user_metadata, switch_nexthop_t nexthop_index) {
        acl_nexthop = nexthop_index;
        local_md.user_metadata = user_metadata;
        stats.count();
    }
    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    table acl {
        key = {
            local_md.lkp.ip_src_addr            : ternary;
            local_md.lkp.ip_dst_addr            : ternary;
            local_md.lkp.ip_proto               : ternary;
            local_md.lkp.ip_tos                 : ternary;
            local_md.lkp.l4_src_port            : ternary;
            local_md.lkp.l4_dst_port            : ternary;
            local_md.lkp.ip_ttl                 : ternary;
            local_md.lkp.ip_frag                : ternary;
            local_md.lkp.tcp_flags              : ternary;
            local_md.l4_src_port_label          : ternary;
            local_md.l4_dst_port_label          : ternary;
            local_md.lkp.mac_type               : ternary;
            local_md.ingress_port_lag_label     : ternary;
            local_md.bd                         : ternary;
            local_md.in_ports_group_label_mirror: ternary;
        }
        actions = {
            drop;
            trap;
            copy_to_cpu;
            permit;
            redirect_nexthop;
            redirect_port;
            mirror_in;
            mirror_out;
            no_action;
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_ACL != 0)) {
            acl.apply();
        }
    }
}

control IngressIpv4Acl(inout switch_local_metadata_t local_md, out switch_nexthop_t acl_nexthop)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action set_tc(switch_tc_t tc) {
        local_md.qos.tc = tc;
        stats.count();
    }
    action set_color(switch_pkt_color_t color) {
        local_md.qos.color = color;
        stats.count();
    }
    action redirect_port(switch_user_metadata_t user_metadata, switch_port_lag_index_t egress_port_lag_index) {
        local_md.flags.acl_deny = false;
        local_md.egress_port_lag_index = egress_port_lag_index;
        local_md.acl_port_redirect = true;
        local_md.user_metadata = user_metadata;
        stats.count();
    }
    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_INGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    action no_action() {
        stats.count();
    }
    action permit(switch_user_metadata_t user_metadata, switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.flags.acl_deny = false;
        local_md.user_metadata = user_metadata;
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }
    action copy_to_cpu(switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action trap(switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        drop();
    }
    action redirect_nexthop(switch_user_metadata_t user_metadata, switch_nexthop_t nexthop_index) {
        acl_nexthop = nexthop_index;
        local_md.user_metadata = user_metadata;
        stats.count();
    }
    action disable_nat(bool nat_dis) {
        local_md.nat.nat_disable = nat_dis;
        stats.count();
    }
    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    table acl {
        key = {
            local_md.lkp.ip_src_addr[95:64]: ternary;
            local_md.lkp.ip_dst_addr[95:64]: ternary;
            local_md.lkp.ip_proto          : ternary;
            local_md.lkp.ip_tos            : ternary;
            local_md.lkp.l4_src_port       : ternary;
            local_md.lkp.l4_dst_port       : ternary;
            local_md.lkp.ip_ttl            : ternary;
            local_md.lkp.ip_frag           : ternary;
            local_md.lkp.tcp_flags         : ternary;
            local_md.l4_src_port_label     : ternary;
            local_md.l4_dst_port_label     : ternary;
            local_md.lkp.mac_type          : ternary;
            local_md.ingress_port_lag_label: ternary;
            local_md.bd                    : ternary;
        }
        actions = {
            drop;
            trap;
            copy_to_cpu;
            permit;
            disable_nat;
            redirect_nexthop;
            redirect_port;
            mirror_in;
            mirror_out;
            no_action;
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_ACL != 0)) {
            acl.apply();
        }
    }
}

control IngressIpv6Acl(inout switch_local_metadata_t local_md, out switch_nexthop_t acl_nexthop)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action set_tc(switch_tc_t tc) {
        local_md.qos.tc = tc;
        stats.count();
    }
    action set_color(switch_pkt_color_t color) {
        local_md.qos.color = color;
        stats.count();
    }
    action redirect_port(switch_user_metadata_t user_metadata, switch_port_lag_index_t egress_port_lag_index) {
        local_md.flags.acl_deny = false;
        local_md.egress_port_lag_index = egress_port_lag_index;
        local_md.acl_port_redirect = true;
        local_md.user_metadata = user_metadata;
        stats.count();
    }
    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_INGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    action no_action() {
        stats.count();
    }
    action permit(switch_user_metadata_t user_metadata, switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.flags.acl_deny = false;
        local_md.user_metadata = user_metadata;
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }
    action copy_to_cpu(switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action trap(switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        drop();
    }
    action redirect_nexthop(switch_user_metadata_t user_metadata, switch_nexthop_t nexthop_index) {
        acl_nexthop = nexthop_index;
        local_md.user_metadata = user_metadata;
        stats.count();
    }
    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    table acl {
        key = {
            local_md.lkp.ip_src_addr       : ternary;
            local_md.lkp.ip_dst_addr       : ternary;
            local_md.lkp.ip_proto          : ternary;
            local_md.lkp.ip_tos            : ternary;
            local_md.lkp.l4_src_port       : ternary;
            local_md.lkp.l4_dst_port       : ternary;
            local_md.lkp.ip_ttl            : ternary;
            local_md.lkp.ip_frag           : ternary;
            local_md.lkp.tcp_flags         : ternary;
            local_md.l4_src_port_label     : ternary;
            local_md.l4_dst_port_label     : ternary;
            local_md.lkp.mac_type          : ternary;
            local_md.ingress_port_lag_label: ternary;
            local_md.bd                    : ternary;
        }
        actions = {
            drop;
            trap;
            copy_to_cpu;
            permit;
            redirect_nexthop;
            redirect_port;
            mirror_in;
            mirror_out;
            no_action;
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_ACL != 0)) {
            acl.apply();
        }
    }
}

control IngressTosMirrorAcl(inout switch_local_metadata_t local_md)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action no_action() {
        stats.count();
    }
    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_INGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    table acl {
        key = {
            local_md.lkp.ip_tos            : ternary;
            local_md.ingress_port_lag_label: ternary;
        }
        actions = {
            mirror_in;
            mirror_out;
            no_action;
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_ACL != 0)) {
            acl.apply();
        }
    }
}

control IngressMacAcl(in switch_header_t hdr, inout switch_local_metadata_t local_md, out switch_nexthop_t acl_nexthop)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action set_tc(switch_tc_t tc) {
        local_md.qos.tc = tc;
        stats.count();
    }
    action set_color(switch_pkt_color_t color) {
        local_md.qos.color = color;
        stats.count();
    }
    action redirect_port(switch_user_metadata_t user_metadata, switch_port_lag_index_t egress_port_lag_index) {
        local_md.flags.acl_deny = false;
        local_md.egress_port_lag_index = egress_port_lag_index;
        local_md.acl_port_redirect = true;
        local_md.user_metadata = user_metadata;
        stats.count();
    }
    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_INGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    action no_action() {
        stats.count();
    }
    action permit(switch_user_metadata_t user_metadata, switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.flags.acl_deny = false;
        local_md.user_metadata = user_metadata;
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }
    action copy_to_cpu(switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action trap(switch_hostif_trap_t trap_id, switch_acl_meter_id_t meter_index) {
        local_md.hostif_trap_id = trap_id;
        local_md.qos.acl_meter_index = meter_index;
        drop();
    }
    action redirect_nexthop(switch_user_metadata_t user_metadata, switch_nexthop_t nexthop_index) {
        acl_nexthop = nexthop_index;
        local_md.user_metadata = user_metadata;
        stats.count();
    }
    table acl {
        key = {
            hdr.ethernet.src_addr          : ternary;
            hdr.ethernet.dst_addr          : ternary;
            local_md.lkp.mac_type          : ternary;
            local_md.ingress_port_lag_label: ternary;
            local_md.bd                    : ternary;
        }
        actions = {
            drop;
            permit;
            redirect_nexthop;
            redirect_port;
            mirror_in;
            no_action;
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_ACL != 0)) {
            acl.apply();
        }
    }
}

control LOU(inout switch_local_metadata_t local_md) {
    const switch_uint32_t table_size = 64 * 1024;
    @name(".set_ingress_src_port_label") action set_src_port_label(bit<8> label) {
        local_md.l4_src_port_label = label;
    }
    @name(".set_ingress_dst_port_label") action set_dst_port_label(bit<8> label) {
        local_md.l4_dst_port_label = label;
    }
    @name(".ingress_l4_dst_port") @use_hash_action(1) table l4_dst_port {
        key = {
            local_md.lkp.l4_dst_port: exact;
        }
        actions = {
            set_dst_port_label;
        }
        const default_action = set_dst_port_label(0);
        size = table_size;
    }
    @name(".ingress_l4_src_port") @use_hash_action(1) table l4_src_port {
        key = {
            local_md.lkp.l4_src_port: exact;
        }
        actions = {
            set_src_port_label;
        }
        const default_action = set_src_port_label(0);
        size = table_size;
    }
    @name(".set_tcp_flags") action set_tcp_flags(bit<8> flags) {
        local_md.lkp.tcp_flags = flags;
    }
    @name(".ingress_lou_tcp") table tcp {
        key = {
            local_md.lkp.tcp_flags: exact;
        }
        actions = {
            NoAction;
            set_tcp_flags;
        }
        size = 256;
    }
    apply {
        l4_src_port.apply();
        l4_dst_port.apply();
    }
}

control IngressSystemAcl(in switch_header_t hdr, inout switch_local_metadata_t local_md, inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm, inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr)(switch_uint32_t table_size=512) {
    const switch_uint32_t drop_stats_table_size = 8192;
    DirectCounter<bit<32>>(CounterType_t.PACKETS) stats;
    @name(".ingress_copp_meter") Meter<bit<8>>(1 << 8, MeterType_t.PACKETS) copp_meter;
    DirectCounter<bit<32>>(CounterType_t.PACKETS) copp_stats;
    switch_copp_meter_id_t copp_meter_id;
    @name(".ingress_system_acl_permit") action permit() {
        local_md.drop_reason = SWITCH_DROP_REASON_UNKNOWN;
    }
    @name(".ingress_system_acl_drop") action drop(switch_drop_reason_t drop_reason, bool disable_learning) {
        ig_intr_md_for_dprsr.drop_ctl = 0x1;
        ig_intr_md_for_dprsr.digest_type = (disable_learning ? SWITCH_DIGEST_TYPE_INVALID : ig_intr_md_for_dprsr.digest_type);
        local_md.drop_reason = drop_reason;
        local_md.nat.hit = SWITCH_NAT_HIT_NONE;
    }
    @name(".ingress_system_acl_copy_to_cpu") action copy_to_cpu(switch_cpu_reason_t reason_code, switch_qid_t qid, switch_copp_meter_id_t meter_id, bool disable_learning, bool overwrite_qid) {
        local_md.qos.qid = (overwrite_qid ? qid : local_md.qos.qid);
        ig_intr_md_for_tm.copy_to_cpu = 1w1;
        ig_intr_md_for_dprsr.digest_type = (disable_learning ? SWITCH_DIGEST_TYPE_INVALID : ig_intr_md_for_dprsr.digest_type);
        ig_intr_md_for_tm.packet_color = (bit<2>)copp_meter.execute(meter_id);
        copp_meter_id = meter_id;
        local_md.cpu_reason = reason_code;
        local_md.nat.hit = SWITCH_NAT_HIT_NONE;
        local_md.drop_reason = SWITCH_DROP_REASON_UNKNOWN;
    }
    @name(".ingress_system_acl_copy_sflow_to_cpu") action copy_sflow_to_cpu(switch_cpu_reason_t reason_code, switch_qid_t qid, switch_copp_meter_id_t meter_id, bool disable_learning, bool overwrite_qid) {
        copy_to_cpu(reason_code + (bit<16>)local_md.sflow.session_id, qid, meter_id, disable_learning, overwrite_qid);
    }
    @name(".ingress_system_acl_redirect_to_cpu") action redirect_to_cpu(switch_cpu_reason_t reason_code, switch_qid_t qid, switch_copp_meter_id_t meter_id, bool disable_learning, bool overwrite_qid) {
        ig_intr_md_for_dprsr.drop_ctl = 0b1;
        local_md.flags.redirect_to_cpu = true;
        copy_to_cpu(reason_code, qid, meter_id, disable_learning, overwrite_qid);
    }
    @name(".ingress_system_acl_redirect_sflow_to_cpu") action redirect_sflow_to_cpu(switch_cpu_reason_t reason_code, switch_qid_t qid, switch_copp_meter_id_t meter_id, bool disable_learning, bool overwrite_qid) {
        ig_intr_md_for_dprsr.drop_ctl = 0b1;
        local_md.flags.redirect_to_cpu = true;
        copy_sflow_to_cpu(reason_code, qid, meter_id, disable_learning, overwrite_qid);
    }
    @name(".ingress_system_acl") table system_acl {
        key = {
            local_md.ingress_port_lag_label        : ternary;
            local_md.bd                            : ternary;
            local_md.ingress_port_lag_index        : ternary;
            local_md.lkp.pkt_type                  : ternary;
            local_md.lkp.mac_type                  : ternary;
            local_md.lkp.mac_dst_addr              : ternary;
            local_md.lkp.ip_type                   : ternary;
            local_md.lkp.ip_ttl                    : ternary;
            local_md.lkp.ip_proto                  : ternary;
            local_md.lkp.ip_frag                   : ternary;
            local_md.lkp.ip_dst_addr               : ternary;
            local_md.lkp.l4_src_port               : ternary;
            local_md.lkp.l4_dst_port               : ternary;
            local_md.lkp.arp_opcode                : ternary;
            local_md.flags.vlan_arp_suppress       : ternary;
            local_md.flags.vrf_ttl_violation       : ternary;
            local_md.flags.vrf_ttl_violation_valid : ternary;
            local_md.flags.vrf_ip_options_violation: ternary;
            local_md.flags.port_vlan_miss          : ternary;
            local_md.flags.acl_deny                : ternary;
            local_md.flags.rmac_hit                : ternary;
            local_md.flags.dmac_miss               : ternary;
            local_md.flags.myip                    : ternary;
            local_md.flags.glean                   : ternary;
            local_md.flags.routed                  : ternary;
            local_md.flags.fib_lpm_miss            : ternary;
            local_md.flags.fib_drop                : ternary;
            local_md.flags.link_local              : ternary;
            local_md.flags.pfc_wd_drop             : ternary;
            local_md.ipv4.unicast_enable           : ternary;
            local_md.ipv6.unicast_enable           : ternary;
            local_md.sflow.sample_packet           : ternary;
            local_md.l2_drop_reason                : ternary;
            local_md.drop_reason                   : ternary;
            local_md.nat.hit                       : ternary;
            local_md.checks.same_zone_check        : ternary;
            local_md.tunnel.terminate              : ternary;
            local_md.hostif_trap_id                : ternary;
            hdr.ipv4_option.isValid()              : ternary;
        }
        actions = {
            permit;
            drop;
            copy_to_cpu;
            redirect_to_cpu;
            copy_sflow_to_cpu;
            redirect_sflow_to_cpu;
        }
        const default_action = permit;
        size = table_size;
    }
    @name(".ingress_copp_drop") action copp_drop() {
        ig_intr_md_for_tm.copy_to_cpu = 1w0;
        copp_stats.count();
    }
    @name(".ingress_copp_permit") action copp_permit() {
        copp_stats.count();
    }
    @name(".ingress_copp") table copp {
        key = {
            ig_intr_md_for_tm.packet_color: ternary;
            copp_meter_id                 : ternary;
        }
        actions = {
            copp_permit;
            copp_drop;
        }
        const default_action = copp_permit;
        size = 1 << 8 + 1;
        counters = copp_stats;
    }
    @name(".ingress_drop_stats_count") action count() {
        stats.count();
    }
    @name(".ingress_drop_stats") table drop_stats {
        key = {
            local_md.drop_reason : exact @name("drop_reason");
            local_md.ingress_port: exact @name("port");
        }
        actions = {
            @defaultonly NoAction;
            count;
        }
        const default_action = NoAction;
        counters = stats;
        size = drop_stats_table_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_SYSTEM_ACL != 0)) {
            switch (system_acl.apply().action_run) {
                copy_to_cpu: {
                    copp.apply();
                }
                redirect_to_cpu: {
                    copp.apply();
                }
                default: {
                }
            }
        }
        drop_stats.apply();
    }
}

control EgressLOU(inout switch_local_metadata_t local_md) {
    const switch_uint32_t table_size = 64 * 1024;
    @name(".set_egress_src_port_label") action set_src_port_label(bit<8> label) {
        local_md.l4_src_port_label = label;
    }
    @name(".set_egress_dst_port_label") action set_dst_port_label(bit<8> label) {
        local_md.l4_dst_port_label = label;
    }
    @name(".egress_l4_dst_port") @use_hash_action(1) table l4_dst_port {
        key = {
            local_md.lkp.l4_dst_port: exact;
        }
        actions = {
            set_dst_port_label;
        }
        const default_action = set_dst_port_label(0);
        size = table_size;
    }
    @name(".egress_l4_src_port") @use_hash_action(1) table l4_src_port {
        key = {
            local_md.lkp.l4_src_port: exact;
        }
        actions = {
            set_src_port_label;
        }
        const default_action = set_src_port_label(0);
        size = table_size;
    }
    apply {
        l4_src_port.apply();
        l4_dst_port.apply();
    }
}

control EgressMacAcl(in switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action no_action() {
        stats.count();
    }
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }
    action permit(switch_acl_meter_id_t meter_index) {
        local_md.flags.acl_deny = false;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    table acl {
        key = {
            hdr.ethernet.src_addr         : ternary;
            hdr.ethernet.dst_addr         : ternary;
            hdr.ethernet.ether_type       : ternary;
            local_md.egress_port_lag_label: ternary;
        }
        actions = {
            drop();
            permit();
            mirror_out();
            no_action;
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
        }
    }
}

control EgressIpv4Acl(in switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action no_action() {
        stats.count();
    }
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }
    action permit(switch_acl_meter_id_t meter_index) {
        local_md.flags.acl_deny = false;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 7;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS_IN_PKT;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    table acl {
        key = {
            hdr.ipv4.src_addr                  : ternary;
            hdr.ipv4.dst_addr                  : ternary;
            hdr.ipv4.protocol                  : ternary;
            local_md.lkp.tcp_flags             : ternary;
            local_md.lkp.l4_src_port           : ternary;
            local_md.lkp.l4_dst_port           : ternary;
            local_md.egress_port_lag_label     : ternary;
            hdr.ethernet.ether_type            : ternary;
            local_md.l4_src_port_label         : ternary;
            local_md.l4_dst_port_label         : ternary;
            hdr.ipv4.diffserv                  : ternary;
            local_md.out_ports_group_label_ipv4: ternary;
            local_md.bd                        : ternary;
        }
        actions = {
            drop();
            permit();
            mirror_out();
            no_action;
            mirror_in();
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
        }
    }
}

control EgressIpv6Acl(in switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action no_action() {
        stats.count();
    }
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }
    action permit(switch_acl_meter_id_t meter_index) {
        local_md.flags.acl_deny = false;
        local_md.qos.acl_meter_index = meter_index;
        stats.count();
    }
    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 7;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS_IN_PKT;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    table acl {
        key = {
            hdr.ipv6.src_addr                  : ternary;
            hdr.ipv6.dst_addr                  : ternary;
            hdr.ipv6.next_hdr                  : ternary;
            local_md.lkp.tcp_flags             : ternary;
            local_md.lkp.l4_src_port           : ternary;
            local_md.lkp.l4_dst_port           : ternary;
            local_md.egress_port_lag_label     : ternary;
            hdr.ethernet.ether_type            : ternary;
            local_md.l4_src_port_label         : ternary;
            local_md.l4_dst_port_label         : ternary;
            hdr.ipv6.traffic_class             : ternary;
            local_md.out_ports_group_label_ipv6: ternary;
            local_md.bd                        : ternary;
        }
        actions = {
            drop();
            permit();
            mirror_out();
            no_action;
            mirror_in();
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
        }
    }
}

control EgressTosMirrorAcl(in switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    action no_action() {
        stats.count();
    }
    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = 7;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS_IN_PKT;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count();
    }
    table acl {
        key = {
            hdr.ipv4.diffserv             : ternary;
            hdr.ipv6.traffic_class        : ternary;
            hdr.ipv4.isValid()            : ternary;
            hdr.ipv6.isValid()            : ternary;
            local_md.egress_port_lag_label: ternary;
        }
        actions = {
            mirror_out;
            no_action;
            mirror_in();
        }
        const default_action = no_action;
        counters = stats;
        size = table_size;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
        }
    }
}

control EgressSystemAcl(inout switch_header_t hdr, inout switch_local_metadata_t local_md, in egress_intrinsic_metadata_t eg_intr_md, out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr)(switch_uint32_t table_size=512) {
    const switch_uint32_t drop_stats_table_size = 8192;
    DirectCounter<bit<32>>(CounterType_t.PACKETS) stats;
    @name(".egress_copp_meter") Meter<bit<8>>(1 << 8, MeterType_t.PACKETS) copp_meter;
    DirectCounter<bit<32>>(CounterType_t.PACKETS) copp_stats;
    switch_copp_meter_id_t copp_meter_id;
    switch_pkt_color_t copp_color;
    @name(".egress_system_acl_drop") action drop(switch_drop_reason_t reason_code) {
        local_md.drop_reason = reason_code;
        eg_intr_md_for_dprsr.drop_ctl = 0x1;
        local_md.mirror.type = 0;
    }
    @name(".egress_system_acl_ingress_timestamp") action insert_timestamp() {
    }
    @name(".egress_system_acl") table system_acl {
        key = {
            eg_intr_md.egress_port                    : ternary;
            local_md.flags.acl_deny                   : ternary;
            local_md.checks.mtu                       : ternary;
            local_md.flags.wred_drop                  : ternary;
            local_md.flags.pfc_wd_drop                : ternary;
            local_md.flags.port_isolation_packet_drop : ternary;
            local_md.flags.bport_isolation_packet_drop: ternary;
        }
        actions = {
            NoAction;
            drop;
            insert_timestamp;
        }
        const default_action = NoAction;
        size = table_size;
    }
    @name(".egress_drop_stats_count") action count() {
        stats.count();
    }
    @name(".egress_drop_stats") table drop_stats {
        key = {
            local_md.drop_reason  : exact @name("drop_reason");
            eg_intr_md.egress_port: exact @name("port");
        }
        actions = {
            @defaultonly NoAction;
            count;
        }
        const default_action = NoAction;
        counters = stats;
        size = drop_stats_table_size;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            switch (system_acl.apply().action_run) {
                default: {
                }
            }
        }
        drop_stats.apply();
    }
}

control IngressSTP(in switch_local_metadata_t local_md, inout switch_stp_metadata_t stp_md)(bool multiple_stp_enable=false, switch_uint32_t table_size=4096) {
    const bit<32> stp_state_size = 1 << 19;
    Hash<bit<32>>(HashAlgorithm_t.IDENTITY) hash;
    @name(".ingress_stp.stp0") Register<bit<1>, bit<32>>(stp_state_size, 0) stp0;
    RegisterAction<bit<1>, bit<32>, bit<1>>(stp0) stp_check0 = {
        void apply(inout bit<1> val, out bit<1> rv) {
            rv = val;
        }
    };
    @name(".ingress_stp.stp1") Register<bit<1>, bit<32>>(stp_state_size, 0) stp1;
    RegisterAction<bit<1>, bit<32>, bit<1>>(stp1) stp_check1 = {
        void apply(inout bit<1> val, out bit<1> rv) {
            rv = val;
        }
    };
    @name(".ingress_stp.set_stp_state") action set_stp_state(switch_stp_state_t stp_state) {
        stp_md.state_ = stp_state;
    }
    @name(".ingress_stp.mstp") table mstp {
        key = {
            local_md.ingress_port: exact;
            stp_md.group         : exact;
        }
        actions = {
            NoAction;
            set_stp_state;
        }
        size = table_size;
        const default_action = NoAction;
    }
    apply {
    }
}

control EgressSTP(in switch_local_metadata_t local_md, in switch_port_t port, out bool stp_state) {
    @name(".egress_stp.stp") Register<bit<1>, bit<32>>(1 << 19, 0) stp;
    Hash<bit<32>>(HashAlgorithm_t.IDENTITY) hash;
    RegisterAction<bit<1>, bit<32>, bit<1>>(stp) stp_check = {
        void apply(inout bit<1> val, out bit<1> rv) {
            rv = val;
        }
    };
    apply {
    }
}

control SMAC(in mac_addr_t src_addr, in mac_addr_t dst_addr, inout switch_local_metadata_t local_md, inout switch_digest_type_t digest_type)(switch_uint32_t table_size) {
    bool src_miss;
    switch_port_lag_index_t src_move;
    @name(".smac_miss") action smac_miss() {
        src_miss = true;
    }
    @name(".smac_hit") action smac_hit(switch_port_lag_index_t port_lag_index) {
        src_move = local_md.ingress_port_lag_index ^ port_lag_index;
    }
    @name(".smac") table smac {
        key = {
            local_md.bd: exact;
            src_addr   : exact;
        }
        actions = {
            @defaultonly smac_miss;
            smac_hit;
        }
        const default_action = smac_miss;
        size = table_size;
        idle_timeout = true;
    }
    action notify() {
        digest_type = SWITCH_DIGEST_TYPE_MAC_LEARNING;
    }
    table learning {
        key = {
            src_miss: exact;
            src_move: ternary;
        }
        actions = {
            NoAction;
            notify;
        }
        const default_action = NoAction;
        const entries = {
                        (true, default) : notify();
                        (false, 0 &&& 0x3ff) : NoAction();
                        (false, default) : notify();
        }
        size = MIN_TABLE_SIZE;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_SMAC != 0)) {
            smac.apply();
        }
        if (local_md.learning.bd_mode == SWITCH_LEARNING_MODE_LEARN && local_md.learning.port_mode == SWITCH_LEARNING_MODE_LEARN) {
            learning.apply();
        }
    }
}

control DMAC_t(in mac_addr_t dst_addr, inout switch_local_metadata_t local_md);
control DMAC(in mac_addr_t dst_addr, inout switch_local_metadata_t local_md)(switch_uint32_t table_size) {
    @name(".dmac_miss") action dmac_miss() {
        local_md.egress_port_lag_index = SWITCH_FLOOD;
        local_md.flags.dmac_miss = true;
    }
    @name(".dmac_hit") action dmac_hit(switch_port_lag_index_t port_lag_index) {
        local_md.egress_port_lag_index = port_lag_index;
    }
    @name(".dmac_redirect") action dmac_redirect(switch_nexthop_t nexthop_index) {
        local_md.nexthop = nexthop_index;
    }
    @pack(2) @name(".dmac") table dmac {
        key = {
            local_md.bd: exact;
            dst_addr   : exact;
        }
        actions = {
            dmac_miss;
            dmac_hit;
            dmac_redirect;
        }
        const default_action = dmac_miss;
        size = table_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_L2 != 0) && local_md.acl_port_redirect == false) {
            dmac.apply();
        }
    }
}

control SameIfCheck(inout switch_local_metadata_t local_md) {
    apply {
        if (local_md.drop_reason == 0 && local_md.ingress_port_lag_index == local_md.egress_port_lag_index) {
            local_md.drop_reason = SWITCH_DROP_REASON_SAME_IFINDEX;
        }
    }
}

control IngressBd(in switch_bd_t bd, in switch_pkt_type_t pkt_type)(switch_uint32_t table_size) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".ingress_bd_stats_count") action count() {
        stats.count();
    }
    @name(".ingress_bd_stats") table bd_stats {
        key = {
            bd      : exact;
            pkt_type: exact;
        }
        actions = {
            count;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        size = 3 * table_size;
        counters = stats;
    }
    apply {
        bd_stats.apply();
    }
}

control EgressBDStats(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".egress_bd_stats_count") action count() {
        stats.count(sizeInBytes(hdr.bridged_md));
    }
    @name(".egress_bd_stats") table bd_stats {
        key = {
            local_md.bd[12:0]    : exact;
            local_md.lkp.pkt_type: exact;
        }
        actions = {
            count;
            @defaultonly NoAction;
        }
        size = 3 * BD_TABLE_SIZE;
        counters = stats;
    }
    apply {
        if (local_md.pkt_src == SWITCH_PKT_SRC_BRIDGED) {
            bd_stats.apply();
        }
    }
}

control EgressBD(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".set_egress_bd_mapping") action set_bd_properties(mac_addr_t smac, switch_mtu_t mtu) {
        hdr.ethernet.src_addr = smac;
        local_md.checks.mtu = mtu;
    }
    @name(".egress_bd_mapping") table bd_mapping {
        key = {
            local_md.bd[12:0]: exact;
        }
        actions = {
            set_bd_properties;
        }
        const default_action = set_bd_properties(0, 0x3fff);
        size = 5120;
    }
    apply {
        if (!local_md.flags.bypass_egress && local_md.flags.routed) {
            bd_mapping.apply();
        }
    }
}

control VlanDecap(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".remove_vlan_tag") action remove_vlan_tag() {
        hdr.ethernet.ether_type = hdr.vlan_tag[0].ether_type;
        hdr.vlan_tag.pop_front(1);
    }
    apply {
        if (!local_md.flags.bypass_egress && hdr.vlan_tag[0].isValid()) {
            hdr.ethernet.ether_type = hdr.vlan_tag[0].ether_type;
            hdr.vlan_tag[0].setInvalid();
            local_md.pkt_length = local_md.pkt_length - 4;
        }
    }
}

control VlanXlate(inout switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t bd_table_size, switch_uint32_t port_bd_table_size) {
    @name(".set_vlan_untagged") action set_vlan_untagged() {
        local_md.lkp.mac_type = hdr.ethernet.ether_type;
    }
    @name(".set_vlan_tagged") action set_vlan_tagged(vlan_id_t vid) {
        hdr.vlan_tag[0].setValid();
        hdr.vlan_tag[0].ether_type = hdr.ethernet.ether_type;
        hdr.vlan_tag[0].dei = 0;
        hdr.vlan_tag[0].vid = vid;
        hdr.ethernet.ether_type = 0x8100;
        hdr.vlan_tag[0].pcp = local_md.qos.pcp;
        local_md.lkp.mac_type = hdr.ethernet.ether_type;
    }
    @name(".port_bd_to_vlan_mapping") table port_bd_to_vlan_mapping {
        key = {
            local_md.egress_port_lag_index: exact @name("port_lag_index");
            local_md.bd                   : exact @name("bd");
        }
        actions = {
            set_vlan_untagged;
            set_vlan_tagged;
        }
        const default_action = set_vlan_untagged;
        size = port_bd_table_size;
    }
    @name(".bd_to_vlan_mapping") table bd_to_vlan_mapping {
        key = {
            local_md.bd: exact @name("bd");
        }
        actions = {
            set_vlan_untagged;
            set_vlan_tagged;
        }
        const default_action = set_vlan_untagged;
        size = bd_table_size;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            if (!port_bd_to_vlan_mapping.apply().hit) {
                bd_to_vlan_mapping.apply();
            }
        }
    }
}

action fib_hit(inout switch_local_metadata_t local_md, switch_nexthop_t nexthop_index, switch_fib_label_t fib_label) {
    local_md.nexthop = nexthop_index;
    local_md.flags.routed = true;
}
action fib_miss(inout switch_local_metadata_t local_md) {
    local_md.flags.routed = false;
}
action fib_miss_lpm4(inout switch_local_metadata_t local_md) {
    local_md.flags.routed = false;
    local_md.flags.fib_lpm_miss = true;
}
action fib_miss_lpm6(inout switch_local_metadata_t local_md) {
    local_md.flags.routed = false;
    local_md.flags.fib_lpm_miss = true;
}
action fib_drop(inout switch_local_metadata_t local_md) {
    local_md.flags.routed = false;
    local_md.flags.fib_drop = true;
}
action fib_myip(inout switch_local_metadata_t local_md, switch_myip_type_t myip) {
    local_md.flags.myip = myip;
}
control Fibv4(in bit<32> dst_addr, inout switch_local_metadata_t local_md)(switch_uint32_t host_table_size, switch_uint32_t lpm_table_size, bool local_host_enable=false, switch_uint32_t local_host_table_size=1024) {
    @pack(2) @name(".ipv4_host") table host {
        key = {
            local_md.vrf: exact @name("vrf");
            dst_addr    : exact @name("ip_dst_addr");
        }
        actions = {
            fib_miss(local_md);
            fib_hit(local_md);
            fib_myip(local_md);
            fib_drop(local_md);
        }
        const default_action = fib_miss(local_md);
        size = host_table_size;
    }
    @name(".ipv4_local_host") table local_host {
        key = {
            local_md.vrf: exact @name("vrf");
            dst_addr    : exact @name("ip_dst_addr");
        }
        actions = {
            fib_miss(local_md);
            fib_hit(local_md);
            fib_myip(local_md);
            fib_drop(local_md);
        }
        const default_action = fib_miss(local_md);
        size = local_host_table_size;
    }
    Alpm(number_partitions = 4096, subtrees_per_partition = 2, atcam_subset_width = 18, shift_granularity = 1) algo_lpm;
    @name(".ipv4_lpm") table lpm32 {
        key = {
            local_md.vrf: exact @name("vrf");
            dst_addr    : lpm @name("ip_dst_addr");
        }
        actions = {
            fib_miss_lpm4(local_md);
            fib_hit(local_md);
            fib_myip(local_md);
            fib_drop(local_md);
        }
        const default_action = fib_miss_lpm4(local_md);
        size = lpm_table_size;
        implementation = algo_lpm;
        requires_versioning = false;
    }
    apply {
        if (local_host_enable) {
            if (!local_host.apply().hit) {
                if (!host.apply().hit) {
                    lpm32.apply();
                }
            }
        } else {
            if (!host.apply().hit) {
                lpm32.apply();
            }
        }
    }
}

control Fibv6(in bit<128> dst_addr, inout switch_local_metadata_t local_md)(switch_uint32_t host_table_size, switch_uint32_t host64_table_size, switch_uint32_t lpm_table_size, switch_uint32_t lpm64_table_size=1024) {
    @name(".ipv6_host") table host {
        key = {
            local_md.vrf: exact @name("vrf");
            dst_addr    : exact @name("ip_dst_addr");
        }
        actions = {
            fib_miss(local_md);
            fib_hit(local_md);
            fib_myip(local_md);
            fib_drop(local_md);
        }
        const default_action = fib_miss(local_md);
        size = host_table_size;
    }
    @name(".ipv6_host64") table host64 {
        key = {
            local_md.vrf    : exact @name("vrf");
            dst_addr[127:64]: exact @name("ip_dst_addr");
        }
        actions = {
            fib_miss(local_md);
            fib_hit(local_md);
            fib_myip(local_md);
            fib_drop(local_md);
        }
        const default_action = fib_miss(local_md);
        size = host64_table_size;
    }
    @name(".ipv6_lpm_tcam") table lpm_tcam {
        key = {
            local_md.vrf: exact @name("vrf");
            dst_addr    : lpm @name("ip_dst_addr");
        }
        actions = {
            fib_miss(local_md);
            fib_hit(local_md);
            fib_myip(local_md);
            fib_drop(local_md);
        }
        const default_action = fib_miss(local_md);
        size = lpm_table_size;
    }
    Alpm(number_partitions = 4096, subtrees_per_partition = 2, atcam_subset_width = 32, shift_granularity = 2) algo_lpm64;
    @name(".ipv6_lpm64") table lpm64 {
        key = {
            local_md.vrf    : exact @name("vrf");
            dst_addr[127:64]: lpm @name("ip_dst_addr");
        }
        actions = {
            fib_miss_lpm6(local_md);
            fib_hit(local_md);
            fib_myip(local_md);
            fib_drop(local_md);
        }
        const default_action = fib_miss_lpm6(local_md);
        size = lpm64_table_size;
        implementation = algo_lpm64;
        requires_versioning = false;
    }
    apply {
        if (!host.apply().hit) {
            if (!lpm_tcam.apply().hit) {
                if (!host64.apply().hit) {
                    lpm64.apply();
                }
            }
        }
    }
}

control EgressVRF(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".set_vrf_properties") action set_vrf_properties(mac_addr_t smac) {
        hdr.ethernet.src_addr = smac;
    }
    @use_hash_action(1) @name(".vrf_mapping") table vrf_mapping {
        key = {
            local_md.vrf: exact @name("vrf");
        }
        actions = {
            set_vrf_properties;
        }
        const default_action = set_vrf_properties(0);
        size = VRF_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress && local_md.flags.routed) {
            vrf_mapping.apply();
            if (hdr.ipv4.isValid()) {
                hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
            } else if (hdr.ipv6.isValid()) {
                hdr.ipv6.hop_limit = hdr.ipv6.hop_limit - 1;
            }
        }
    }
}

control MTU(in switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=16) {
    action ipv4_mtu_check() {
        local_md.checks.mtu = local_md.checks.mtu |-| hdr.ipv4.total_len;
    }
    action ipv6_mtu_check() {
        local_md.checks.mtu = local_md.checks.mtu |-| hdr.ipv6.payload_len;
    }
    action mtu_miss() {
        local_md.checks.mtu = 16w0xffff;
    }
    table mtu {
        key = {
            local_md.flags.bypass_egress: exact;
            local_md.flags.routed       : exact;
            hdr.ipv4.isValid()          : exact;
            hdr.ipv6.isValid()          : exact;
        }
        actions = {
            ipv4_mtu_check;
            ipv6_mtu_check;
            mtu_miss;
        }
        const default_action = mtu_miss;
        const entries = {
                        (false, true, true, false) : ipv4_mtu_check();
                        (false, true, false, true) : ipv6_mtu_check();
        }
        size = table_size;
    }
    apply {
        mtu.apply();
    }
}

control Nexthop(inout switch_local_metadata_t local_md)(switch_uint32_t nexthop_table_size, switch_uint32_t ecmp_group_table_size, switch_uint32_t ecmp_selection_table_size, switch_uint32_t ecmp_max_members_per_group=64) {
    Hash<switch_uint16_t>(HashAlgorithm_t.IDENTITY) selector_hash;
    @name(".nexthop_ecmp_action_profile") ActionProfile(ecmp_selection_table_size) ecmp_action_profile;
    @name(".nexthop_ecmp_selector") ActionSelector(ecmp_action_profile, selector_hash, SelectorMode_t.FAIR, ecmp_max_members_per_group, ecmp_group_table_size) ecmp_selector;
    @name(".nexthop_set_nexthop_properties") action set_nexthop_properties(switch_port_lag_index_t port_lag_index, switch_nat_zone_t zone) {
        local_md.checks.same_zone_check = local_md.nat.ingress_zone ^ zone;
        local_md.egress_port_lag_index = port_lag_index;
        local_md.tunnel_nexthop = local_md.nexthop;
    }
    @name(".set_ecmp_properties") action set_ecmp_properties(switch_port_lag_index_t port_lag_index, switch_nexthop_t nexthop_index, switch_nat_zone_t zone) {
        local_md.nexthop = nexthop_index;
        local_md.tunnel_nexthop = local_md.nexthop;
        set_nexthop_properties(port_lag_index, zone);
    }
    @name(".set_nexthop_properties_post_routed_flood") action set_nexthop_properties_post_routed_flood(switch_bd_t bd, switch_mgid_t mgid, switch_nat_zone_t zone) {
        local_md.egress_port_lag_index = 0;
        local_md.multicast.id = mgid;
        local_md.checks.same_zone_check = local_md.nat.ingress_zone ^ zone;
    }
    @name(".set_nexthop_properties_glean") action set_nexthop_properties_glean(switch_hostif_trap_t trap_id) {
        local_md.flags.glean = true;
        local_md.hostif_trap_id = trap_id;
    }
    @name(".set_ecmp_properties_glean") action set_ecmp_properties_glean(switch_hostif_trap_t trap_id) {
        set_nexthop_properties_glean(trap_id);
    }
    @name(".set_nexthop_properties_drop") action set_nexthop_properties_drop(switch_drop_reason_t drop_reason) {
        local_md.drop_reason = drop_reason;
    }
    @name(".set_ecmp_properties_drop") action set_ecmp_properties_drop() {
        set_nexthop_properties_drop(SWITCH_DROP_REASON_NEXTHOP);
    }
    @name(".set_nexthop_properties_tunnel") action set_nexthop_properties_tunnel(switch_tunnel_ip_index_t dip_index) {
        local_md.tunnel.dip_index = dip_index;
        local_md.egress_port_lag_index = 0;
        local_md.tunnel_nexthop = local_md.nexthop;
    }
    @name(".set_ecmp_properties_tunnel") action set_ecmp_properties_tunnel(switch_tunnel_ip_index_t dip_index, switch_nexthop_t nexthop_index) {
        local_md.tunnel.dip_index = dip_index;
        local_md.egress_port_lag_index = 0;
        local_md.tunnel_nexthop = nexthop_index;
    }
    @ways(2) @name(".ecmp_table") table ecmp {
        key = {
            local_md.nexthop  : exact;
            local_md.ecmp_hash: selector;
        }
        actions = {
            @defaultonly NoAction;
            set_ecmp_properties;
            set_ecmp_properties_drop;
            set_ecmp_properties_glean;
            set_ecmp_properties_tunnel;
        }
        const default_action = NoAction;
        size = ecmp_group_table_size;
        implementation = ecmp_selector;
    }
    @name(".nexthop") table nexthop {
        key = {
            local_md.nexthop: exact;
        }
        actions = {
            @defaultonly NoAction;
            set_nexthop_properties;
            set_nexthop_properties_drop;
            set_nexthop_properties_glean;
            set_nexthop_properties_post_routed_flood;
            set_nexthop_properties_tunnel;
        }
        const default_action = NoAction;
        size = nexthop_table_size;
    }
    apply {
        if (local_md.acl_port_redirect == true) {
            local_md.flags.routed = false;
            local_md.nexthop = 0;
        } else {
            if (local_md.acl_nexthop != 0) {
                local_md.flags.fib_lpm_miss = false;
                local_md.nexthop = local_md.acl_nexthop;
            }
            switch (nexthop.apply().action_run) {
                NoAction: {
                    ecmp.apply();
                }
                default: {
                }
            }
        }
    }
}

control OuterFib(inout switch_local_metadata_t local_md)(switch_uint32_t ecmp_max_members_per_group=64) {
    Hash<switch_uint16_t>(HashAlgorithm_t.IDENTITY) selector_hash;
    @name(".outer_fib_ecmp_action_profile") ActionProfile(ECMP_SELECT_TABLE_SIZE) ecmp_action_profile;
    @name(".outer_fib_ecmp_selector") ActionSelector(ecmp_action_profile, selector_hash, SelectorMode_t.FAIR, ecmp_max_members_per_group, ECMP_GROUP_TABLE_SIZE) ecmp_selector;
    @name(".outer_fib_set_nexthop_properties") action set_nexthop_properties(switch_port_lag_index_t port_lag_index, switch_nexthop_t nexthop_index) {
        local_md.nexthop = nexthop_index;
        local_md.egress_port_lag_index = port_lag_index;
    }
    @name(".outer_fib") table fib {
        key = {
            local_md.tunnel.dip_index: exact;
            local_md.outer_ecmp_hash : selector;
        }
        actions = {
            NoAction;
            set_nexthop_properties;
        }
        const default_action = NoAction;
        implementation = ecmp_selector;
        size = 1 << 15;
    }
    apply {
        fib.apply();
    }
}

control Neighbor(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".neighbor_rewrite_l2") action rewrite_l2(mac_addr_t dmac) {
        hdr.ethernet.dst_addr = dmac;
    }
    @use_hash_action(1) @name(".neighbor") table neighbor {
        key = {
            local_md.nexthop: exact;
        }
        actions = {
            rewrite_l2;
        }
        const default_action = rewrite_l2(0);
        size = NEXTHOP_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress && local_md.flags.routed && local_md.nexthop != 0) {
            neighbor.apply();
        }
    }
}

control OuterNexthop(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".outer_nexthop_rewrite_l2") action rewrite_l2(switch_bd_t bd) {
        local_md.bd = bd;
    }
    @use_hash_action(1) @name(".outer_nexthop") table outer_nexthop {
        key = {
            local_md.nexthop: exact;
        }
        actions = {
            rewrite_l2;
        }
        const default_action = rewrite_l2(0);
        size = NEXTHOP_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress && local_md.flags.routed && local_md.nexthop != 0) {
            outer_nexthop.apply();
        }
    }
}

parser SwitchIngressParser(packet_in pkt, out switch_header_t hdr, out switch_local_metadata_t local_md, out ingress_intrinsic_metadata_t ig_intr_md) {
    Checksum() ipv4_checksum;
    Checksum() inner_ipv4_checksum;
    Checksum() inner2_ipv4_checksum;
    @name(".ingress_udp_port_vxlan") value_set<bit<16>>(1) udp_port_vxlan;
    @name(".ingress_cpu_port") value_set<switch_cpu_port_value_set_t>(1) cpu_port;
    @name(".ingress_nvgre_st_key") value_set<switch_nvgre_value_set_t>(1) nvgre_st_key;
    Checksum() tcp_checksum;
    Checksum() udp_checksum;
    state start {
        pkt.extract(ig_intr_md);
        local_md.ingress_port = ig_intr_md.ingress_port;
        local_md.timestamp = ig_intr_md.ingress_mac_tstamp[31:0];
        transition parse_port_metadata;
    }
    state parse_resubmit {
        transition accept;
    }
    state parse_port_metadata {
        switch_port_metadata_t port_md = port_metadata_unpack<switch_port_metadata_t>(pkt);
        local_md.ingress_port_lag_index = port_md.port_lag_index;
        local_md.ingress_port_lag_label = port_md.port_lag_label;
        transition parse_packet;
    }
    state parse_packet {
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type, local_md.ingress_port) {
            cpu_port: parse_cpu;
            (0x800, default): parse_ipv4;
            (0x806, default): parse_arp;
            (0x86dd, default): parse_ipv6;
            (0x8100, default): parse_vlan;
            default: accept;
        }
    }
    state parse_cpu {
        pkt.extract(hdr.fabric);
        pkt.extract(hdr.cpu);
        local_md.bypass = hdr.cpu.reason_code;
        transition select(hdr.cpu.ether_type) {
            0x800: parse_ipv4;
            0x806: parse_arp;
            0x86dd: parse_ipv6;
            0x8100: parse_vlan;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        ipv4_checksum.add(hdr.ipv4);
        tcp_checksum.subtract({ hdr.ipv4.src_addr, hdr.ipv4.dst_addr });
        udp_checksum.subtract({ hdr.ipv4.src_addr, hdr.ipv4.dst_addr });
        transition select(hdr.ipv4.ihl) {
            5: parse_ipv4_no_options;
            6: parse_ipv4_options;
            default: accept;
        }
    }
    state parse_ipv4_options {
        pkt.extract(hdr.ipv4_option);
        ipv4_checksum.add(hdr.ipv4_option);
        transition parse_ipv4_no_options;
    }
    state parse_ipv4_no_options {
        local_md.flags.ipv4_checksum_err = ipv4_checksum.verify();
        transition select(hdr.ipv4.protocol, hdr.ipv4.frag_offset) {
            (1, 0): parse_icmp;
            (2, 0): parse_igmp;
            (6, 0): parse_tcp;
            (17, 0): parse_udp;
            (4, 0): parse_ipinip;
            (41, 0): parse_ipv6inip;
            default: accept;
        }
    }
    state parse_arp {
        pkt.extract(hdr.arp);
        transition accept;
    }
    state parse_vlan {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            0x806: parse_arp;
            0x800: parse_ipv4;
            0x8100: parse_vlan;
            0x86dd: parse_ipv6;
            default: accept;
        }
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        tcp_checksum.subtract({ hdr.ipv6.src_addr, hdr.ipv6.dst_addr });
        udp_checksum.subtract({ hdr.ipv6.src_addr, hdr.ipv6.dst_addr });
        transition select(hdr.ipv6.next_hdr) {
            58: parse_icmp;
            6: parse_tcp;
            17: parse_udp;
            4: parse_ipinip;
            41: parse_ipv6inip;
            default: accept;
        }
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        udp_checksum.subtract_all_and_deposit(local_md.tcp_udp_checksum);
        udp_checksum.subtract({ hdr.udp.checksum });
        udp_checksum.subtract({ hdr.udp.src_port, hdr.udp.dst_port });
        transition select(hdr.udp.dst_port) {
            2123: parse_gtp_u;
            udp_port_vxlan: parse_vxlan;
            4791: parse_rocev2;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        tcp_checksum.subtract_all_and_deposit(local_md.tcp_udp_checksum);
        tcp_checksum.subtract({ hdr.tcp.checksum });
        tcp_checksum.subtract({ hdr.tcp.src_port, hdr.tcp.dst_port });
        transition accept;
    }
    state parse_icmp {
        pkt.extract(hdr.icmp);
        transition accept;
    }
    state parse_igmp {
        pkt.extract(hdr.igmp);
        transition accept;
    }
    state parse_gtp_u {
        pkt.extract(hdr.gtp);
        transition accept;
    }
    state parse_rocev2 {
        transition accept;
    }
    state parse_vxlan {
        pkt.extract(hdr.vxlan);
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_VXLAN;
        local_md.tunnel.vni = hdr.vxlan.vni;
        transition parse_inner_ethernet;
    }
    state parse_ipinip {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_IPINIP;
        transition parse_inner_ipv4;
    }
    state parse_ipv6inip {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_IPINIP;
        transition parse_inner_ipv6;
    }
    state parse_inner_ethernet {
        pkt.extract(hdr.inner_ethernet);
        transition select(hdr.inner_ethernet.ether_type) {
            0x800: parse_inner_ipv4;
            0x86dd: parse_inner_ipv6;
            default: accept;
        }
    }
    state parse_inner_ipv4 {
        pkt.extract(hdr.inner_ipv4);
        inner_ipv4_checksum.add(hdr.inner_ipv4);
        local_md.flags.inner_ipv4_checksum_err = inner_ipv4_checksum.verify();
        transition select(hdr.inner_ipv4.protocol) {
            1: parse_inner_icmp;
            2: parse_inner_igmp;
            6: parse_inner_tcp;
            17: parse_inner_udp;
            default: accept;
        }
    }
    state parse_inner_ipv6 {
        pkt.extract(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.next_hdr) {
            58: parse_inner_icmp;
            6: parse_inner_tcp;
            17: parse_inner_udp;
            default: accept;
        }
    }
    state parse_inner_udp {
        pkt.extract(hdr.inner_udp);
        transition accept;
    }
    state parse_inner_tcp {
        pkt.extract(hdr.inner_tcp);
        transition accept;
    }
    state parse_inner_icmp {
        pkt.extract(hdr.inner_icmp);
        transition accept;
    }
    state parse_inner_igmp {
        pkt.extract(hdr.inner_igmp);
        transition accept;
    }
}

parser SwitchEgressParser(packet_in pkt, out switch_header_t hdr, out switch_local_metadata_t local_md, out egress_intrinsic_metadata_t eg_intr_md) {
    @name(".egress_udp_port_vxlan") value_set<bit<16>>(1) udp_port_vxlan;
    @name(".egress_cpu_port") value_set<switch_cpu_port_value_set_t>(1) cpu_port;
    @name(".egress_nvgre_st_key") value_set<switch_nvgre_value_set_t>(1) nvgre_st_key;
    @critical state start {
        pkt.extract(eg_intr_md);
        local_md.pkt_length = eg_intr_md.pkt_length;
        local_md.egress_port = eg_intr_md.egress_port;
        local_md.qos.qdepth = eg_intr_md.deq_qdepth;
        switch_port_mirror_metadata_h mirror_md = pkt.lookahead<switch_port_mirror_metadata_h>();
        transition select(eg_intr_md.deflection_flag, mirror_md.src, mirror_md.type) {
            (1, default, default): parse_deflected_pkt;
            (default, SWITCH_PKT_SRC_BRIDGED, default): parse_bridged_pkt;
            (default, SWITCH_PKT_SRC_CLONED_EGRESS_IN_PKT, 7): parse_port_mirrored_with_bridged_metadata;
            (default, default, 1): parse_port_mirrored_metadata;
            (default, SWITCH_PKT_SRC_CLONED_EGRESS, 2): parse_cpu_mirrored_metadata;
            (default, SWITCH_PKT_SRC_CLONED_INGRESS, 3): parse_dtel_drop_metadata_from_ingress;
            (default, default, 3): parse_dtel_drop_metadata_from_egress;
            (default, default, 4): parse_dtel_switch_local_metadata;
            (default, default, 5): parse_simple_mirrored_metadata;
        }
    }
    state parse_bridged_pkt {
        pkt.extract(hdr.bridged_md);
        local_md.pkt_src = SWITCH_PKT_SRC_BRIDGED;
        local_md.ingress_port = hdr.bridged_md.base.ingress_port;
        local_md.egress_port_lag_index = hdr.bridged_md.base.ingress_port_lag_index;
        local_md.bd = hdr.bridged_md.base.ingress_bd;
        local_md.nexthop = hdr.bridged_md.base.nexthop;
        local_md.cpu_reason = hdr.bridged_md.base.cpu_reason;
        local_md.flags.routed = hdr.bridged_md.base.routed;
        local_md.flags.bypass_egress = hdr.bridged_md.base.bypass_egress;
        local_md.lkp.pkt_type = hdr.bridged_md.base.pkt_type;
        local_md.ingress_timestamp = hdr.bridged_md.base.timestamp;
        local_md.qos.tc = hdr.bridged_md.base.tc;
        local_md.qos.qid = hdr.bridged_md.base.qid;
        local_md.qos.color = hdr.bridged_md.base.color;
        local_md.vrf = hdr.bridged_md.base.vrf;
        local_md.lkp.l4_src_port = hdr.bridged_md.acl.l4_src_port;
        local_md.lkp.l4_dst_port = hdr.bridged_md.acl.l4_dst_port;
        local_md.lkp.tcp_flags = hdr.bridged_md.acl.tcp_flags;
        local_md.tunnel_nexthop = hdr.bridged_md.tunnel.tunnel_nexthop;
        local_md.tunnel.hash = hdr.bridged_md.tunnel.hash;
        local_md.tunnel.ttl_mode = hdr.bridged_md.tunnel.ttl_mode;
        local_md.tunnel.qos_mode = hdr.bridged_md.tunnel.qos_mode;
        local_md.tunnel.terminate = hdr.bridged_md.tunnel.terminate;
        local_md.mirror.type = hdr.bridged_md.mirror.type;
        local_md.mirror.src = hdr.bridged_md.mirror.src;
        local_md.mirror.session_id = hdr.bridged_md.mirror.session_id;
        local_md.mirror.meter_index = hdr.bridged_md.mirror.meter_index;
        transition parse_ethernet;
    }
    state parse_deflected_pkt {
        transition reject;
    }
    state parse_port_mirrored_with_bridged_metadata {
        switch_port_mirror_metadata_h port_md;
        switch_bridged_metadata_h b_md;
        pkt.extract(port_md);
        pkt.extract(b_md);
        pkt.extract(hdr.ethernet);
        local_md.pkt_src = port_md.src;
        local_md.mirror.session_id = port_md.session_id;
        local_md.ingress_timestamp = port_md.timestamp;
        local_md.flags.bypass_egress = true;
        local_md.mirror.type = port_md.type;
        local_md.ingress_port = port_md.port;
        transition accept;
    }
    state parse_port_mirrored_metadata {
        switch_port_mirror_metadata_h port_md;
        pkt.extract(port_md);
        pkt.extract(hdr.ethernet);
        local_md.pkt_src = port_md.src;
        local_md.mirror.session_id = port_md.session_id;
        local_md.ingress_timestamp = port_md.timestamp;
        local_md.flags.bypass_egress = true;
        local_md.mirror.type = port_md.type;
        local_md.ingress_port = port_md.port;
        transition accept;
    }
    state parse_cpu_mirrored_metadata {
        switch_cpu_mirror_metadata_h cpu_md;
        pkt.extract(cpu_md);
        pkt.extract(hdr.ethernet);
        local_md.pkt_src = cpu_md.src;
        local_md.flags.bypass_egress = true;
        local_md.bd = cpu_md.bd;
        local_md.cpu_reason = cpu_md.reason_code;
        local_md.mirror.type = cpu_md.type;
        transition accept;
    }
    state parse_dtel_drop_metadata_from_egress {
        transition reject;
    }
    state parse_dtel_drop_metadata_from_ingress {
        transition reject;
    }
    state parse_dtel_switch_local_metadata {
        transition reject;
    }
    state parse_simple_mirrored_metadata {
        transition reject;
    }
    state parse_packet {
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type, eg_intr_md.egress_port) {
            cpu_port: parse_cpu;
            (0x800, default): parse_ipv4;
            (0x86dd, default): parse_ipv6;
            (0x8100, default): parse_vlan;
            default: parse_pad;
        }
    }
    state parse_cpu {
        local_md.flags.bypass_egress = true;
        transition parse_pad;
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol, hdr.ipv4.ihl, hdr.ipv4.frag_offset) {
            (17, 5, 0): parse_udp;
            (4, 5, 0): parse_inner_ipv4;
            (41, 5, 0): parse_inner_ipv6;
            (default, 6, default): parse_ipv4_options;
            default: parse_pad;
        }
    }
    state parse_ipv4_options {
        pkt.extract(hdr.ipv4_option);
        transition select(hdr.ipv4.protocol, hdr.ipv4.frag_offset) {
            (17, 0): parse_udp;
            (4, 0): parse_inner_ipv4;
            (41, 0): parse_inner_ipv6;
            default: parse_pad;
        }
    }
    state parse_vlan {
        pkt.extract(hdr.vlan_tag.next);
        local_md.qos.pcp = hdr.vlan_tag.last.pcp;
        transition select(hdr.vlan_tag.last.ether_type) {
            0x800: parse_ipv4;
            0x8100: parse_vlan;
            0x86dd: parse_ipv6;
            default: parse_pad;
        }
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition select(hdr.ipv6.next_hdr) {
            17: parse_udp;
            4: parse_inner_ipv4;
            41: parse_inner_ipv6;
            default: parse_pad;
        }
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port) {
            udp_port_vxlan: parse_vxlan;
            default: parse_pad;
        }
    }
    state egress_parse_sfc_pause {
        transition parse_pad;
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition parse_pad;
    }
    state parse_vxlan {
        pkt.extract(hdr.vxlan);
        transition parse_inner_ethernet;
    }
    state parse_inner_ethernet {
        pkt.extract(hdr.inner_ethernet);
        transition select(hdr.inner_ethernet.ether_type) {
            0x800: parse_inner_ipv4;
            0x86dd: parse_inner_ipv6;
            default: parse_pad;
        }
    }
    state parse_inner_ipv4 {
        pkt.extract(hdr.inner_ipv4);
        transition parse_pad;
    }
    state parse_inner_ipv6 {
        pkt.extract(hdr.inner_ipv6);
        transition parse_pad;
    }
    state parse_pad {
        transition accept;
    }
}

control IngressMirror(inout switch_header_t hdr, in switch_local_metadata_t local_md, in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr) {
    Mirror() mirror;
    apply {
        if (ig_intr_md_for_dprsr.mirror_type == 1) {
            mirror.emit<switch_port_mirror_metadata_h>(local_md.mirror.session_id, { local_md.mirror.src, local_md.mirror.type, local_md.timestamp, local_md.mirror.session_id, 0, local_md.ingress_port });
        } else if (ig_intr_md_for_dprsr.mirror_type == 3) {
        } else if (ig_intr_md_for_dprsr.mirror_type == 6) {
        }
    }
}

control EgressMirror(inout switch_header_t hdr, in switch_local_metadata_t local_md, in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {
    Mirror() mirror;
    apply {
        if (eg_intr_md_for_dprsr.mirror_type == 1) {
            mirror.emit<switch_port_mirror_metadata_h>(local_md.mirror.session_id, { local_md.mirror.src, local_md.mirror.type, local_md.ingress_timestamp, local_md.mirror.session_id, 0, local_md.egress_port });
        } else if (eg_intr_md_for_dprsr.mirror_type == 7) {
            mirror.emit<switch_port_mirror_metadata_h>(local_md.mirror.session_id, { local_md.mirror.src, local_md.mirror.type, local_md.ingress_timestamp, local_md.mirror.session_id, 0, local_md.ingress_port });
        } else if (eg_intr_md_for_dprsr.mirror_type == 2) {
            mirror.emit<switch_cpu_mirror_metadata_h>(local_md.mirror.session_id, { local_md.mirror.src, local_md.mirror.type, 0, local_md.ingress_port, local_md.bd, local_md.egress_port_lag_index, local_md.cpu_reason });
        } else if (eg_intr_md_for_dprsr.mirror_type == 4) {
        } else if (eg_intr_md_for_dprsr.mirror_type == 3) {
        } else if (eg_intr_md_for_dprsr.mirror_type == 5) {
        }
    }
}

control IngressNatChecksum(inout switch_header_t hdr, in switch_local_metadata_t local_md) {
    Checksum() tcp_checksum;
    Checksum() udp_checksum;
    apply {
        if (hdr.tcp.isValid()) {
            hdr.tcp.checksum = tcp_checksum.update({ local_md.lkp.ip_src_addr, local_md.lkp.ip_dst_addr, hdr.tcp.src_port, hdr.tcp.dst_port, local_md.tcp_udp_checksum });
        } else if (hdr.udp.isValid()) {
            hdr.udp.checksum = udp_checksum.update({ local_md.lkp.ip_src_addr, local_md.lkp.ip_dst_addr, hdr.udp.src_port, hdr.udp.dst_port, local_md.tcp_udp_checksum });
        }
    }
}

control SwitchIngressDeparser(packet_out pkt, inout switch_header_t hdr, in switch_local_metadata_t local_md, in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr) {
    IngressMirror() mirror;
    @name(".learning_digest") Digest<switch_learning_digest_t>() digest;
    IngressNatChecksum() nat_checksum;
    apply {
        mirror.apply(hdr, local_md, ig_intr_md_for_dprsr);
        if (ig_intr_md_for_dprsr.digest_type == SWITCH_DIGEST_TYPE_MAC_LEARNING) {
            digest.pack({ local_md.ingress_outer_bd, local_md.ingress_port_lag_index, hdr.ethernet.src_addr });
        }
        nat_checksum.apply(hdr, local_md);
        pkt.emit(hdr.bridged_md);
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.vlan_tag);
        pkt.emit(hdr.arp);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv4_option);
        pkt.emit(hdr.ipv6);
        pkt.emit(hdr.udp);
        pkt.emit(hdr.tcp);
        pkt.emit(hdr.icmp);
        pkt.emit(hdr.igmp);
        pkt.emit(hdr.rocev2_bth);
        pkt.emit(hdr.vxlan);
        pkt.emit(hdr.gre);
        pkt.emit(hdr.inner_ethernet);
        pkt.emit(hdr.inner_ipv4);
        pkt.emit(hdr.inner_ipv6);
        pkt.emit(hdr.inner_udp);
        pkt.emit(hdr.inner_tcp);
        pkt.emit(hdr.inner_icmp);
    }
}

control SwitchEgressDeparser(packet_out pkt, inout switch_header_t hdr, in switch_local_metadata_t local_md, in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {
    EgressMirror() mirror;
    Checksum() ipv4_checksum;
    Checksum() inner_ipv4_checksum;
    apply {
        mirror.apply(hdr, local_md, eg_intr_md_for_dprsr);
        hdr.ipv4.hdr_checksum = ipv4_checksum.update({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.ipv4_option.type, hdr.ipv4_option.length, hdr.ipv4_option.value });
        if (local_md.inner_ipv4_checksum_update_en) {
            hdr.inner_ipv4.hdr_checksum = inner_ipv4_checksum.update({ hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.total_len, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.frag_offset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.src_addr, hdr.inner_ipv4.dst_addr });
        }
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.fabric);
        pkt.emit(hdr.cpu);
        pkt.emit(hdr.timestamp);
        pkt.emit(hdr.vlan_tag);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv4_option);
        pkt.emit(hdr.ipv6);
        pkt.emit(hdr.udp);
        pkt.emit(hdr.dtel);
        pkt.emit(hdr.dtel_report);
        pkt.emit(hdr.dtel_switch_local_report);
        pkt.emit(hdr.dtel_drop_report);
        pkt.emit(hdr.vxlan);
        pkt.emit(hdr.gre);
        pkt.emit(hdr.erspan);
        pkt.emit(hdr.erspan_type2);
        pkt.emit(hdr.erspan_type3);
        pkt.emit(hdr.erspan_platform);
        pkt.emit(hdr.inner_ethernet);
        pkt.emit(hdr.inner_ipv4);
        pkt.emit(hdr.inner_ipv6);
        pkt.emit(hdr.inner_udp);
        pkt.emit(hdr.pad);
    }
}

control MirrorRewrite(inout switch_header_t hdr, inout switch_local_metadata_t local_md, out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr)(switch_uint32_t table_size=1024) {
    bit<16> length;
    action add_ethernet_header(in mac_addr_t src_addr, in mac_addr_t dst_addr, in bit<16> ether_type) {
        hdr.ethernet.setValid();
        hdr.ethernet.ether_type = ether_type;
        hdr.ethernet.src_addr = src_addr;
        hdr.ethernet.dst_addr = dst_addr;
    }
    action add_vlan_tag(vlan_id_t vid, bit<3> pcp, bit<16> ether_type) {
        hdr.vlan_tag[0].setValid();
        hdr.vlan_tag[0].pcp = pcp;
        hdr.vlan_tag[0].vid = vid;
        hdr.vlan_tag[0].ether_type = ether_type;
    }
    action add_ipv4_header(in bit<8> diffserv, in bit<8> ttl, in bit<8> protocol, in ipv4_addr_t src_addr, in ipv4_addr_t dst_addr) {
        hdr.ipv4.setValid();
        hdr.ipv4.version = 4w4;
        hdr.ipv4.ihl = 4w5;
        hdr.ipv4.diffserv = diffserv;
        hdr.ipv4.identification = 0;
        hdr.ipv4.flags = 0;
        hdr.ipv4.frag_offset = 0;
        hdr.ipv4.ttl = ttl;
        hdr.ipv4.protocol = protocol;
        hdr.ipv4.src_addr = src_addr;
        hdr.ipv4.dst_addr = dst_addr;
        hdr.ipv6.setInvalid();
    }
    action add_gre_header(in bit<16> proto) {
        hdr.gre.setValid();
        hdr.gre.proto = proto;
        hdr.gre.flags_version = 0;
    }
    action add_erspan_common(bit<16> version_vlan, bit<10> session_id) {
        hdr.erspan.setValid();
        hdr.erspan.version_vlan = version_vlan;
        hdr.erspan.session_id = (bit<16>)session_id;
    }
    action add_erspan_type2(bit<10> session_id) {
        add_erspan_common(0x1000, session_id);
        hdr.erspan_type2.setValid();
        hdr.erspan_type2.index = 0;
    }
    action add_erspan_type3(bit<10> session_id, bit<32> timestamp) {
        add_erspan_common(0x2000, session_id);
        hdr.erspan_type3.setValid();
        hdr.erspan_type3.timestamp = timestamp;
        hdr.erspan_type3.ft_d_other = 0x4;
    }
    @name(".rewrite_") action rewrite_(switch_qid_t qid) {
        local_md.qos.qid = qid;
    }
    @name(".rewrite_erspan_type2") action rewrite_erspan_type2(switch_qid_t qid, mac_addr_t smac, mac_addr_t dmac, ipv4_addr_t sip, ipv4_addr_t dip, bit<8> tos, bit<8> ttl) {
        local_md.qos.qid = qid;
        add_erspan_type2((bit<10>)local_md.mirror.session_id);
        add_gre_header(0x88be);
        add_ipv4_header(tos, ttl, 47, sip, dip);
        hdr.ipv4.total_len = local_md.pkt_length + 16w32;
        hdr.inner_ethernet = hdr.ethernet;
        add_ethernet_header(smac, dmac, 0x800);
    }
    @name(".rewrite_erspan_type2_with_vlan") action rewrite_erspan_type2_with_vlan(switch_qid_t qid, bit<16> ether_type, mac_addr_t smac, mac_addr_t dmac, bit<3> pcp, vlan_id_t vid, ipv4_addr_t sip, ipv4_addr_t dip, bit<8> tos, bit<8> ttl) {
        local_md.qos.qid = qid;
        add_erspan_type2((bit<10>)local_md.mirror.session_id);
        add_gre_header(0x88be);
        add_ipv4_header(tos, ttl, 47, sip, dip);
        hdr.ipv4.total_len = local_md.pkt_length + 16w32;
        hdr.inner_ethernet = hdr.ethernet;
        add_ethernet_header(smac, dmac, ether_type);
        add_vlan_tag(vid, pcp, 0x800);
    }
    @name(".rewrite_erspan_type3") action rewrite_erspan_type3(switch_qid_t qid, mac_addr_t smac, mac_addr_t dmac, ipv4_addr_t sip, ipv4_addr_t dip, bit<8> tos, bit<8> ttl) {
        local_md.qos.qid = qid;
        add_erspan_type3((bit<10>)local_md.mirror.session_id, (bit<32>)local_md.ingress_timestamp);
        add_gre_header(0x22eb);
        add_ipv4_header(tos, ttl, 47, sip, dip);
        hdr.ipv4.total_len = local_md.pkt_length + 16w36;
        hdr.inner_ethernet = hdr.ethernet;
        add_ethernet_header(smac, dmac, 0x800);
    }
    @name(".rewrite_erspan_type3_with_vlan") action rewrite_erspan_type3_with_vlan(switch_qid_t qid, bit<16> ether_type, mac_addr_t smac, mac_addr_t dmac, bit<3> pcp, vlan_id_t vid, ipv4_addr_t sip, ipv4_addr_t dip, bit<8> tos, bit<8> ttl) {
        local_md.qos.qid = qid;
        add_erspan_type3((bit<10>)local_md.mirror.session_id, (bit<32>)local_md.ingress_timestamp);
        add_gre_header(0x22eb);
        add_ipv4_header(tos, ttl, 47, sip, dip);
        hdr.ipv4.total_len = local_md.pkt_length + 16w36;
        hdr.inner_ethernet = hdr.ethernet;
        add_ethernet_header(smac, dmac, ether_type);
        add_vlan_tag(vid, pcp, 0x800);
    }
    @name(".mirror_rewrite") table rewrite {
        key = {
            local_md.mirror.session_id: exact;
        }
        actions = {
            NoAction;
            rewrite_;
            rewrite_erspan_type2;
            rewrite_erspan_type2_with_vlan;
            rewrite_erspan_type3;
            rewrite_erspan_type3_with_vlan;
        }
        const default_action = NoAction;
        size = table_size;
    }
    action adjust_length(bit<16> length_offset) {
        local_md.pkt_length = local_md.pkt_length + length_offset;
        local_md.mirror.type = 0;
    }
    action adjust_length_with_b_md() {
        switch_bridged_metadata_h b_md;
        switch_pkt_length_t length_offset = 0xfff3 - (bit<16>)sizeInBytes(b_md);
        local_md.pkt_length = local_md.pkt_length + length_offset;
        local_md.mirror.type = 0;
    }
    table pkt_length {
        key = {
            local_md.mirror.type: exact;
        }
        actions = {
            adjust_length;
            adjust_length_with_b_md;
        }
        const entries = {
                        2 : adjust_length(0xfff2);
                        1 : adjust_length(0xfff3);
                        3 : adjust_length(2);
                        4 : adjust_length(0x0);
                        5 : adjust_length(0xfff9);
                        7 : adjust_length_with_b_md();
                        255 : adjust_length(20);
        }
    }
    action rewrite_ipv4_udp_len_truncate() {
    }
    apply {
        if (local_md.pkt_src != SWITCH_PKT_SRC_BRIDGED) {
            pkt_length.apply();
            rewrite.apply();
        }
    }
}

control IngressRmac(inout switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t port_vlan_table_size, switch_uint32_t vlan_table_size=4096) {
    @name(".rmac_miss") action rmac_miss() {
    }
    @name(".rmac_hit") action rmac_hit() {
        local_md.flags.rmac_hit = true;
    }
    @name(".pv_rmac") table pv_rmac {
        key = {
            local_md.ingress_port_lag_index: ternary;
            hdr.vlan_tag[0].isValid()      : ternary;
            hdr.vlan_tag[0].vid            : ternary;
            hdr.ethernet.dst_addr          : ternary;
        }
        actions = {
            rmac_miss;
            rmac_hit;
        }
        const default_action = rmac_miss;
        size = port_vlan_table_size;
    }
    @name(".vlan_rmac") table vlan_rmac {
        key = {
            hdr.vlan_tag[0].vid  : exact;
            hdr.ethernet.dst_addr: exact;
        }
        actions = {
            @defaultonly rmac_miss;
            rmac_hit;
        }
        const default_action = rmac_miss;
        size = vlan_table_size;
    }
    apply {
        switch (pv_rmac.apply().action_run) {
            rmac_miss: {
                if (hdr.vlan_tag[0].isValid()) {
                    vlan_rmac.apply();
                }
            }
        }
    }
}

control IngressPortMirror(in switch_port_t port, inout switch_mirror_metadata_t mirror_md)(switch_uint32_t table_size=288) {
    @name(".set_ingress_mirror_id") action set_ingress_mirror_id(switch_mirror_session_t session_id, switch_mirror_meter_id_t meter_index) {
        mirror_md.type = 1;
        mirror_md.src = SWITCH_PKT_SRC_CLONED_INGRESS;
        mirror_md.session_id = session_id;
        mirror_md.meter_index = (switch_mirror_meter_id_t)meter_index;
    }
    @name(".ingress_port_mirror") table ingress_port_mirror {
        key = {
            port: exact;
        }
        actions = {
            NoAction;
            set_ingress_mirror_id;
        }
        const default_action = NoAction;
        size = table_size;
    }
    apply {
        ingress_port_mirror.apply();
    }
}

control EgressPortMirror(in switch_port_t port, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=288) {
    @name(".set_egress_mirror_id") action set_egress_mirror_id(switch_mirror_session_t session_id, switch_mirror_meter_id_t meter_index) {
        local_md.mirror.type = 1;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = (switch_mirror_meter_id_t)meter_index;
    }
    @name(".egress_port_mirror") table egress_port_mirror {
        key = {
            port: exact;
        }
        actions = {
            NoAction;
            set_egress_mirror_id;
        }
        const default_action = NoAction;
        size = table_size;
    }
    apply {
        egress_port_mirror.apply();
    }
}

control IngressPortMapping(inout switch_header_t hdr, inout switch_local_metadata_t local_md, inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm, inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr)(switch_uint32_t port_vlan_table_size, switch_uint32_t bd_table_size, switch_uint32_t port_table_size=288, switch_uint32_t vlan_table_size=4096, switch_uint32_t double_tag_table_size=1024) {
    IngressPortMirror(port_table_size) port_mirror;
    IngressRmac(port_vlan_table_size, vlan_table_size) rmac;
    @name(".bd_action_profile") ActionProfile(bd_table_size) bd_action_profile;
    Hash<bit<32>>(HashAlgorithm_t.IDENTITY) hash;
    const bit<32> vlan_membership_size = 1 << 19;
    @name(".vlan_membership") Register<bit<1>, bit<32>>(vlan_membership_size, 0) vlan_membership;
    RegisterAction<bit<1>, bit<32>, bit<1>>(vlan_membership) check_vlan_membership = {
        void apply(inout bit<1> val, out bit<1> rv) {
            rv = ~val;
        }
    };
    action terminate_cpu_packet() {
        local_md.ingress_port = (switch_port_t)hdr.cpu.ingress_port;
        ig_intr_md_for_tm.qid = (switch_qid_t)hdr.cpu.egress_queue;
        local_md.egress_port_lag_index = (switch_port_lag_index_t)hdr.cpu.port_lag_index;
        local_md.flags.bypass_egress = (bool)hdr.cpu.tx_bypass;
        hdr.ethernet.ether_type = hdr.cpu.ether_type;
    }
    @name(".set_cpu_port_properties") action set_cpu_port_properties(switch_port_lag_index_t port_lag_index, switch_ig_port_lag_label_t port_lag_label, switch_yid_t exclusion_id, switch_pkt_color_t color, switch_tc_t tc) {
        local_md.ingress_port_lag_index = port_lag_index;
        local_md.ingress_port_lag_label = port_lag_label;
        local_md.qos.color = color;
        local_md.qos.tc = tc;
        ig_intr_md_for_tm.level2_exclusion_id = exclusion_id;
    }
    @name(".set_port_properties") action set_port_properties(switch_yid_t exclusion_id, switch_learning_mode_t learning_mode, switch_pkt_color_t color, switch_tc_t tc, switch_port_meter_id_t meter_index, switch_sflow_id_t sflow_session_id, bool mac_pkt_class, switch_ports_group_label_t in_ports_group_label_ipv4, switch_ports_group_label_t in_ports_group_label_ipv6, switch_ports_group_label_t in_ports_group_label_mirror) {
        local_md.qos.color = color;
        local_md.qos.tc = tc;
        ig_intr_md_for_tm.level2_exclusion_id = exclusion_id;
        local_md.learning.port_mode = learning_mode;
        local_md.flags.mac_pkt_class = mac_pkt_class;
        local_md.sflow.session_id = sflow_session_id;
        local_md.in_ports_group_label_mirror = in_ports_group_label_mirror;
    }
    @placement_priority(2) @name(".ingress_port_mapping") table port_mapping {
        key = {
            local_md.ingress_port: exact;
        }
        actions = {
            set_port_properties;
            set_cpu_port_properties;
        }
        size = port_table_size;
    }
    @name(".port_vlan_miss") action port_vlan_miss() {
    }
    @name(".set_bd_properties") action set_bd_properties(switch_bd_t bd, switch_vrf_t vrf, bool vlan_arp_suppress, switch_packet_action_t vrf_ttl_violation, bool vrf_ttl_violation_valid, switch_packet_action_t vrf_ip_options_violation, bool vrf_unknown_l3_multicast_trap, switch_bd_label_t bd_label, switch_stp_group_t stp_group, switch_learning_mode_t learning_mode, bool ipv4_unicast_enable, bool ipv4_multicast_enable, bool igmp_snooping_enable, bool ipv6_unicast_enable, bool ipv6_multicast_enable, bool mld_snooping_enable, bool mpls_enable, switch_multicast_rpf_group_t mrpf_group, switch_nat_zone_t zone) {
        local_md.bd = bd;
        local_md.flags.vlan_arp_suppress = vlan_arp_suppress;
        local_md.ingress_outer_bd = bd;
        local_md.vrf = vrf;
        local_md.flags.vrf_ttl_violation = vrf_ttl_violation;
        local_md.flags.vrf_ttl_violation_valid = vrf_ttl_violation_valid;
        local_md.flags.vrf_ip_options_violation = vrf_ip_options_violation;
        local_md.flags.vrf_unknown_l3_multicast_trap = vrf_unknown_l3_multicast_trap;
        local_md.stp.group = stp_group;
        local_md.multicast.rpf_group = mrpf_group;
        local_md.learning.bd_mode = learning_mode;
        local_md.ipv4.unicast_enable = ipv4_unicast_enable;
        local_md.ipv4.multicast_enable = ipv4_multicast_enable;
        local_md.ipv4.multicast_snooping = igmp_snooping_enable;
        local_md.ipv6.unicast_enable = ipv6_unicast_enable;
        local_md.ipv6.multicast_enable = ipv6_multicast_enable;
        local_md.ipv6.multicast_snooping = mld_snooping_enable;
        local_md.nat.ingress_zone = zone;
    }
    @placement_priority(2) @name(".port_vlan_to_bd_mapping") table port_vlan_to_bd_mapping {
        key = {
            local_md.ingress_port_lag_index: ternary;
            hdr.vlan_tag[0].isValid()      : ternary;
            hdr.vlan_tag[0].vid            : ternary;
        }
        actions = {
            NoAction;
            port_vlan_miss;
            set_bd_properties;
        }
        const default_action = NoAction;
        implementation = bd_action_profile;
        size = port_vlan_table_size;
    }
    @placement_priority(2) @name(".vlan_to_bd_mapping") table vlan_to_bd_mapping {
        key = {
            hdr.vlan_tag[0].vid: exact;
        }
        actions = {
            NoAction;
            port_vlan_miss;
            set_bd_properties;
        }
        const default_action = port_vlan_miss;
        implementation = bd_action_profile;
        size = vlan_table_size;
    }
    @name(".cpu_to_bd_mapping") table cpu_to_bd_mapping {
        key = {
            hdr.cpu.ingress_bd: exact;
        }
        actions = {
            NoAction;
            port_vlan_miss;
            set_bd_properties;
        }
        const default_action = port_vlan_miss;
        implementation = bd_action_profile;
        size = bd_table_size;
    }
    apply {
        port_mirror.apply(local_md.ingress_port, local_md.mirror);
        switch (port_mapping.apply().action_run) {
            set_port_properties: {
                if (!port_vlan_to_bd_mapping.apply().hit) {
                    if (hdr.vlan_tag[0].isValid()) {
                        vlan_to_bd_mapping.apply();
                    }
                }
                rmac.apply(hdr, local_md);
            }
        }
        if (hdr.vlan_tag[0].isValid() && !hdr.vlan_tag[1].isValid() && (bit<1>)local_md.flags.port_vlan_miss == 0) {
            bit<32> pv_hash_ = hash.get({ local_md.ingress_port[6:0], hdr.vlan_tag[0].vid });
            local_md.flags.port_vlan_miss = (bool)check_vlan_membership.execute(pv_hash_);
        }
    }
}

control IngressPortStats(in switch_header_t hdr, in switch_port_t port, in bit<1> drop, in bit<1> copy_to_cpu) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".ingress_ip_stats_count") action no_action() {
        stats.count();
    }
    @name(".ingress_ip_port_stats") table ingress_ip_port_stats {
        key = {
            port                 : exact;
            hdr.ipv4.isValid()   : ternary;
            hdr.ipv6.isValid()   : ternary;
            drop                 : ternary;
            copy_to_cpu          : ternary;
            hdr.ethernet.dst_addr: ternary;
        }
        actions = {
            no_action;
        }
        const default_action = no_action;
        size = 512;
        counters = stats;
    }
    apply {
        ingress_ip_port_stats.apply();
    }
}

control EgressPortStats(in switch_header_t hdr, in switch_port_t port, in bit<1> drop, in bit<1> copy_to_cpu) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".egress_ip_stats_count") action no_action() {
        stats.count();
    }
    @name(".egress_ip_port_stats") table egress_ip_port_stats {
        key = {
            port                 : exact;
            hdr.ipv4.isValid()   : ternary;
            hdr.ipv6.isValid()   : ternary;
            drop                 : ternary;
            copy_to_cpu          : ternary;
            hdr.ethernet.dst_addr: ternary;
        }
        actions = {
            no_action;
        }
        const default_action = no_action;
        size = 512;
        counters = stats;
    }
    apply {
        egress_ip_port_stats.apply();
    }
}

control LAG(inout switch_local_metadata_t local_md, in switch_hash_t hash, out switch_port_t egress_port) {
    Hash<switch_uint16_t>(HashAlgorithm_t.CRC16) selector_hash;
    @name(".lag_action_profile") ActionProfile(LAG_SELECTOR_TABLE_SIZE) lag_action_profile;
    @name(".lag_selector") ActionSelector(lag_action_profile, selector_hash, SelectorMode_t.FAIR, LAG_MAX_MEMBERS_PER_GROUP, LAG_GROUP_TABLE_SIZE) lag_selector;
    @name(".set_lag_port") action set_lag_port(switch_port_t port) {
        egress_port = port;
    }
    @name(".lag_miss") action lag_miss() {
    }
    @name(".lag_table") table lag {
        key = {
            local_md.egress_port_lag_index: exact @name("port_lag_index");
            hash                          : selector;
        }
        actions = {
            lag_miss;
            set_lag_port;
        }
        const default_action = lag_miss;
        size = LAG_TABLE_SIZE;
        implementation = lag_selector;
    }
    apply {
        lag.apply();
    }
}

control EgressPortMapping(inout switch_header_t hdr, inout switch_local_metadata_t local_md, inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr, in switch_port_t port)(switch_uint32_t table_size=288) {
    @name(".set_port_normal") action set_port_normal(switch_port_lag_index_t port_lag_index, switch_eg_port_lag_label_t port_lag_label, switch_port_meter_id_t meter_index, switch_sflow_id_t sflow_session_id, switch_ports_group_label_t out_ports_group_label_ipv4, switch_ports_group_label_t out_ports_group_label_ipv6) {
        local_md.egress_port_lag_index = port_lag_index;
        local_md.egress_port_lag_label = port_lag_label;
        local_md.out_ports_group_label_ipv4 = out_ports_group_label_ipv4;
        local_md.out_ports_group_label_ipv6 = out_ports_group_label_ipv6;
    }
    @name(".set_port_cpu") action set_port_cpu() {
        local_md.flags.to_cpu = true;
    }
    @name(".egress_port_mapping") table port_mapping {
        key = {
            port: exact;
        }
        actions = {
            set_port_normal;
            set_port_cpu;
        }
        size = table_size;
    }
    @name(".set_egress_ingress_port_properties") action set_egress_ingress_port_properties(switch_isolation_group_t port_isolation_group, switch_isolation_group_t bport_isolation_group) {
        local_md.port_isolation_group = port_isolation_group;
        local_md.bport_isolation_group = bport_isolation_group;
    }
    @name(".egress_ingress_port_mapping") table egress_ingress_port_mapping {
        key = {
            local_md.ingress_port: exact;
        }
        actions = {
            NoAction;
            set_egress_ingress_port_properties;
        }
        const default_action = NoAction;
        size = table_size;
    }
    apply {
        port_mapping.apply();
        egress_ingress_port_mapping.apply();
    }
}

control EgressPortIsolation(inout switch_local_metadata_t local_md, in egress_intrinsic_metadata_t eg_intr_md)(switch_uint32_t table_size=288) {
    @name(".isolate_packet_port") action isolate_packet_port(bool drop) {
        local_md.flags.port_isolation_packet_drop = drop;
    }
    @name(".egress_port_isolation") table egress_port_isolation {
        key = {
            eg_intr_md.egress_port       : exact;
            local_md.port_isolation_group: exact;
        }
        actions = {
            isolate_packet_port;
        }
        const default_action = isolate_packet_port(false);
        size = 1024;
    }
    @name(".isolate_packet_bport") action isolate_packet_bport(bool drop) {
        local_md.flags.bport_isolation_packet_drop = drop;
    }
    @name(".egress_bport_isolation") table egress_bport_isolation {
        key = {
            eg_intr_md.egress_port        : exact;
            local_md.flags.routed         : exact;
            local_md.bport_isolation_group: exact;
        }
        actions = {
            isolate_packet_bport;
        }
        const default_action = isolate_packet_bport(false);
        size = 1024;
    }
    apply {
        egress_port_isolation.apply();
        egress_bport_isolation.apply();
    }
}

control EgressCpuRewrite(inout switch_header_t hdr, inout switch_local_metadata_t local_md, inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr, in switch_port_t port)(switch_uint32_t table_size=288) {
    @name(".cpu_rewrite") action cpu_rewrite() {
        hdr.fabric.setValid();
        hdr.fabric.reserved = 0;
        hdr.fabric.color = 0;
        hdr.fabric.qos = 0;
        hdr.fabric.reserved2 = 0;
        hdr.cpu.setValid();
        hdr.cpu.egress_queue = 0;
        hdr.cpu.tx_bypass = 0;
        hdr.cpu.capture_ts = 0;
        hdr.cpu.reserved = 0;
        hdr.cpu.ingress_port = (bit<16>)local_md.ingress_port;
        hdr.cpu.port_lag_index = (bit<16>)local_md.egress_port_lag_index;
        hdr.cpu.ingress_bd = (bit<16>)local_md.bd;
        hdr.cpu.reason_code = local_md.cpu_reason;
        hdr.cpu.ether_type = hdr.ethernet.ether_type;
        hdr.ethernet.ether_type = 0x9000;
    }
    @name(".cpu_port_rewrite") table cpu_port_rewrite {
        key = {
            port: exact;
        }
        actions = {
            cpu_rewrite;
        }
        size = table_size;
    }
    apply {
        cpu_port_rewrite.apply();
    }
}

control PktValidation(in switch_header_t hdr, inout switch_local_metadata_t local_md) {
    const switch_uint32_t table_size = MIN_TABLE_SIZE;
    action valid_ethernet_pkt(switch_pkt_type_t pkt_type) {
        local_md.lkp.pkt_type = pkt_type;
        local_md.lkp.mac_dst_addr = hdr.ethernet.dst_addr;
    }
    @name(".malformed_eth_pkt") action malformed_eth_pkt(bit<8> reason) {
        local_md.lkp.mac_dst_addr = hdr.ethernet.dst_addr;
        local_md.lkp.mac_type = hdr.ethernet.ether_type;
        local_md.l2_drop_reason = reason;
    }
    @name(".valid_pkt_untagged") action valid_pkt_untagged(switch_pkt_type_t pkt_type) {
        local_md.lkp.mac_type = hdr.ethernet.ether_type;
        valid_ethernet_pkt(pkt_type);
    }
    @name(".valid_pkt_tagged") action valid_pkt_tagged(switch_pkt_type_t pkt_type) {
        local_md.lkp.mac_type = hdr.vlan_tag[0].ether_type;
        local_md.lkp.pcp = hdr.vlan_tag[0].pcp;
        valid_ethernet_pkt(pkt_type);
    }
    @name(".validate_ethernet") table validate_ethernet {
        key = {
            hdr.ethernet.src_addr    : ternary;
            hdr.ethernet.dst_addr    : ternary;
            hdr.vlan_tag[0].isValid(): ternary;
        }
        actions = {
            malformed_eth_pkt;
            valid_pkt_untagged;
            valid_pkt_tagged;
        }
        size = table_size;
    }
    @name(".valid_arp_pkt") action valid_arp_pkt() {
        local_md.lkp.arp_opcode = hdr.arp.opcode;
    }
    @name(".valid_ipv6_pkt") action valid_ipv6_pkt(bool is_link_local) {
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
        local_md.lkp.ip_tos = hdr.ipv6.traffic_class;
        local_md.lkp.ip_proto = hdr.ipv6.next_hdr;
        local_md.lkp.ip_ttl = hdr.ipv6.hop_limit;
        local_md.lkp.ip_src_addr = hdr.ipv6.src_addr;
        local_md.lkp.ip_dst_addr = hdr.ipv6.dst_addr;
        local_md.lkp.ipv6_flow_label = hdr.ipv6.flow_label;
        local_md.flags.link_local = is_link_local;
    }
    @name(".valid_ipv4_pkt") action valid_ipv4_pkt(switch_ip_frag_t ip_frag, bool is_link_local) {
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
        local_md.lkp.ip_tos = hdr.ipv4.diffserv;
        local_md.lkp.ip_proto = hdr.ipv4.protocol;
        local_md.lkp.ip_ttl = hdr.ipv4.ttl;
        local_md.lkp.ip_src_addr[63:0] = 64w0;
        local_md.lkp.ip_src_addr[95:64] = hdr.ipv4.src_addr;
        local_md.lkp.ip_src_addr[127:96] = 32w0;
        local_md.lkp.ip_dst_addr[63:0] = 64w0;
        local_md.lkp.ip_dst_addr[95:64] = hdr.ipv4.dst_addr;
        local_md.lkp.ip_dst_addr[127:96] = 32w0;
        local_md.lkp.ip_frag = ip_frag;
        local_md.flags.link_local = is_link_local;
    }
    @name(".malformed_ipv4_pkt") action malformed_ipv4_pkt(bit<8> reason, switch_ip_frag_t ip_frag) {
        valid_ipv4_pkt(ip_frag, false);
        local_md.drop_reason = reason;
    }
    @name(".malformed_ipv6_pkt") action malformed_ipv6_pkt(bit<8> reason) {
        valid_ipv6_pkt(false);
        local_md.drop_reason = reason;
    }
    @name(".validate_ip") table validate_ip {
        key = {
            hdr.arp.isValid()               : ternary;
            hdr.ipv4.isValid()              : ternary;
            local_md.flags.ipv4_checksum_err: ternary;
            hdr.ipv4.version                : ternary;
            hdr.ipv4.ihl                    : ternary;
            hdr.ipv4.flags                  : ternary;
            hdr.ipv4.frag_offset            : ternary;
            hdr.ipv4.ttl                    : ternary;
            hdr.ipv4.src_addr[31:0]         : ternary;
            hdr.ipv6.isValid()              : ternary;
            hdr.ipv6.version                : ternary;
            hdr.ipv6.hop_limit              : ternary;
            hdr.ipv6.src_addr[127:0]        : ternary;
        }
        actions = {
            malformed_ipv4_pkt;
            malformed_ipv6_pkt;
            valid_arp_pkt;
            valid_ipv4_pkt;
            valid_ipv6_pkt;
        }
        size = table_size;
    }
    action set_tcp_ports() {
        local_md.lkp.l4_src_port = hdr.tcp.src_port;
        local_md.lkp.l4_dst_port = hdr.tcp.dst_port;
        local_md.lkp.tcp_flags = hdr.tcp.flags;
    }
    action set_udp_ports() {
        local_md.lkp.l4_src_port = hdr.udp.src_port;
        local_md.lkp.l4_dst_port = hdr.udp.dst_port;
        local_md.lkp.tcp_flags = 0;
    }
    action set_icmp_type() {
        local_md.lkp.l4_src_port[7:0] = hdr.icmp.type;
        local_md.lkp.l4_src_port[15:8] = hdr.icmp.code;
        local_md.lkp.l4_dst_port = 0;
        local_md.lkp.tcp_flags = 0;
    }
    action set_igmp_type() {
        local_md.lkp.l4_src_port[7:0] = hdr.igmp.type;
        local_md.lkp.l4_src_port[15:8] = 0;
        local_md.lkp.l4_dst_port = 0;
        local_md.lkp.tcp_flags = 0;
    }
    table validate_other {
        key = {
            hdr.tcp.isValid() : exact;
            hdr.udp.isValid() : exact;
            hdr.icmp.isValid(): exact;
            hdr.igmp.isValid(): exact;
        }
        actions = {
            NoAction;
            set_tcp_ports;
            set_udp_ports;
            set_icmp_type;
        }
        const default_action = NoAction;
        const entries = {
                        (true, false, false, false) : set_tcp_ports();
                        (false, true, false, false) : set_udp_ports();
                        (false, false, true, false) : set_icmp_type();
        }
        size = 16;
    }
    apply {
        validate_ethernet.apply();
        validate_ip.apply();
        validate_other.apply();
    }
}

control SameMacCheck(in switch_header_t hdr, inout switch_local_metadata_t local_md) {
    apply {
        if (hdr.ethernet.src_addr[31:0] == hdr.ethernet.dst_addr[31:0]) {
            if (hdr.ethernet.src_addr[47:32] == hdr.ethernet.dst_addr[47:32]) {
                local_md.drop_reason = SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK;
            }
        }
    }
}

control InnerPktValidation(in switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".valid_inner_ethernet_pkt") action valid_ethernet_pkt(switch_pkt_type_t pkt_type) {
        local_md.lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;
        local_md.lkp.mac_type = hdr.inner_ethernet.ether_type;
        local_md.lkp.pkt_type = pkt_type;
    }
    @name(".valid_inner_ipv4_pkt") action valid_ipv4_pkt(switch_pkt_type_t pkt_type) {
        local_md.lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;
        local_md.lkp.mac_type = 0x800;
        local_md.lkp.pkt_type = pkt_type;
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
        local_md.lkp.ip_tos = hdr.inner_ipv4.diffserv;
        local_md.lkp.ip_ttl = hdr.inner_ipv4.ttl;
        local_md.lkp.ip_proto = hdr.inner_ipv4.protocol;
        local_md.lkp.ip_src_addr[63:0] = 64w0;
        local_md.lkp.ip_src_addr[95:64] = hdr.inner_ipv4.src_addr;
        local_md.lkp.ip_src_addr[127:96] = 32w0;
        local_md.lkp.ip_dst_addr[63:0] = 64w0;
        local_md.lkp.ip_dst_addr[95:64] = hdr.inner_ipv4.dst_addr;
        local_md.lkp.ip_dst_addr[127:96] = 32w0;
    }
    @name(".valid_inner_ipv6_pkt") action valid_ipv6_pkt(switch_pkt_type_t pkt_type) {
        local_md.lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;
        local_md.lkp.mac_type = 0x86dd;
        local_md.lkp.pkt_type = pkt_type;
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
        local_md.lkp.ip_tos = hdr.inner_ipv6.traffic_class;
        local_md.lkp.ip_ttl = hdr.inner_ipv6.hop_limit;
        local_md.lkp.ip_proto = hdr.inner_ipv6.next_hdr;
        local_md.lkp.ip_src_addr = hdr.inner_ipv6.src_addr;
        local_md.lkp.ip_dst_addr = hdr.inner_ipv6.dst_addr;
        local_md.lkp.ipv6_flow_label = hdr.inner_ipv6.flow_label;
        local_md.flags.link_local = false;
    }
    action set_tcp_ports() {
        local_md.lkp.l4_src_port = hdr.inner_tcp.src_port;
        local_md.lkp.l4_dst_port = hdr.inner_tcp.dst_port;
        local_md.lkp.tcp_flags = hdr.inner_tcp.flags;
    }
    action set_udp_ports() {
        local_md.lkp.l4_src_port = hdr.inner_udp.src_port;
        local_md.lkp.l4_dst_port = hdr.inner_udp.dst_port;
        local_md.lkp.tcp_flags = 0;
    }
    action set_icmp_type() {
        local_md.lkp.l4_src_port[7:0] = hdr.inner_icmp.type;
        local_md.lkp.l4_src_port[15:8] = hdr.inner_icmp.code;
        local_md.lkp.l4_dst_port = 0;
        local_md.lkp.tcp_flags = 0;
    }
    action set_igmp_type() {
        local_md.lkp.l4_src_port[7:0] = hdr.inner_igmp.type;
        local_md.lkp.l4_src_port[15:8] = 0;
        local_md.lkp.l4_dst_port = 0;
        local_md.lkp.tcp_flags = 0;
    }
    @name(".valid_inner_ipv4_tcp_pkt") action valid_ipv4_tcp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv4_pkt(pkt_type);
        set_tcp_ports();
    }
    @name(".valid_inner_ipv4_udp_pkt") action valid_ipv4_udp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv4_pkt(pkt_type);
        set_udp_ports();
    }
    @name(".valid_inner_ipv4_icmp_pkt") action valid_ipv4_icmp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv4_pkt(pkt_type);
        set_icmp_type();
    }
    @name(".valid_inner_ipv4_igmp_pkt") action valid_ipv4_igmp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv4_pkt(pkt_type);
        set_igmp_type();
    }
    @name(".valid_inner_ipv6_tcp_pkt") action valid_ipv6_tcp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv6_pkt(pkt_type);
        set_tcp_ports();
    }
    @name(".valid_inner_ipv6_udp_pkt") action valid_ipv6_udp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv6_pkt(pkt_type);
        set_udp_ports();
    }
    @name(".valid_inner_ipv6_icmp_pkt") action valid_ipv6_icmp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv6_pkt(pkt_type);
        set_icmp_type();
    }
    @name(".malformed_l2_inner_pkt") action malformed_l2_pkt(bit<8> reason) {
        local_md.l2_drop_reason = reason;
    }
    @name(".malformed_l3_inner_pkt") action malformed_l3_pkt(bit<8> reason) {
        local_md.drop_reason = reason;
    }
    @name(".validate_inner_ethernet") table validate_ethernet {
        key = {
            hdr.inner_ethernet.isValid()          : ternary;
            hdr.inner_ethernet.dst_addr           : ternary;
            hdr.inner_ipv6.isValid()              : ternary;
            hdr.inner_ipv6.version                : ternary;
            hdr.inner_ipv6.hop_limit              : ternary;
            hdr.inner_ipv4.isValid()              : ternary;
            local_md.flags.inner_ipv4_checksum_err: ternary;
            hdr.inner_ipv4.version                : ternary;
            hdr.inner_ipv4.ihl                    : ternary;
            hdr.inner_ipv4.ttl                    : ternary;
            hdr.inner_tcp.isValid()               : ternary;
            hdr.inner_udp.isValid()               : ternary;
            hdr.inner_icmp.isValid()              : ternary;
            hdr.inner_igmp.isValid()              : ternary;
        }
        actions = {
            NoAction;
            valid_ipv4_tcp_pkt;
            valid_ipv4_udp_pkt;
            valid_ipv4_icmp_pkt;
            valid_ipv4_igmp_pkt;
            valid_ipv4_pkt;
            valid_ipv6_tcp_pkt;
            valid_ipv6_udp_pkt;
            valid_ipv6_icmp_pkt;
            valid_ipv6_pkt;
            valid_ethernet_pkt;
            malformed_l2_pkt;
            malformed_l3_pkt;
        }
        size = MIN_TABLE_SIZE;
    }
    apply {
        validate_ethernet.apply();
    }
}

control EgressPacketValidation(in switch_header_t hdr, inout switch_local_metadata_t local_md) {
    action valid_ipv4_pkt() {
        local_md.lkp.ip_src_addr[63:0] = 64w0;
        local_md.lkp.ip_src_addr[95:64] = hdr.ipv4.src_addr;
        local_md.lkp.ip_src_addr[127:96] = 32w0;
        local_md.lkp.ip_dst_addr[63:0] = 64w0;
        local_md.lkp.ip_dst_addr[95:64] = hdr.ipv4.dst_addr;
        local_md.lkp.ip_dst_addr[127:96] = 32w0;
        local_md.lkp.ip_tos = hdr.ipv4.diffserv;
        local_md.lkp.ip_proto = hdr.ipv4.protocol;
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
    }
    action valid_ipv6_pkt() {
        local_md.lkp.ip_src_addr = hdr.ipv6.src_addr;
        local_md.lkp.ip_dst_addr = hdr.ipv6.dst_addr;
        local_md.lkp.ip_tos = hdr.ipv6.traffic_class;
        local_md.lkp.ip_proto = hdr.ipv6.next_hdr;
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
    }
    @name(".egress_pkt_validation") table validate_ip {
        key = {
            hdr.ipv4.isValid(): ternary;
            hdr.ipv6.isValid(): ternary;
        }
        actions = {
            valid_ipv4_pkt;
            valid_ipv6_pkt;
        }
        const entries = {
                        (true, false) : valid_ipv4_pkt;
                        (false, true) : valid_ipv6_pkt;
        }
        size = MIN_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            validate_ip.apply();
        }
    }
}

control IngressTunnel(inout switch_header_t hdr, inout switch_local_metadata_t local_md, inout switch_lookup_fields_t lkp) {
    InnerPktValidation() pkt_validation;
    @name(".tunnel_rmac_miss") action rmac_miss() {
        local_md.flags.rmac_hit = false;
    }
    @name(".tunnel_rmac_hit") action rmac_hit() {
        local_md.flags.rmac_hit = true;
    }
    @name(".vxlan_rmac") table vxlan_rmac {
        key = {
            local_md.tunnel.vni        : exact;
            hdr.inner_ethernet.dst_addr: exact;
        }
        actions = {
            @defaultonly rmac_miss;
            rmac_hit;
        }
        const default_action = rmac_miss;
        size = VNI_MAPPING_TABLE_SIZE;
    }
    @name(".vxlan_device_rmac") table vxlan_device_rmac {
        key = {
            hdr.inner_ethernet.dst_addr: exact;
        }
        actions = {
            @defaultonly rmac_miss;
            rmac_hit;
        }
        const default_action = rmac_miss;
        size = 128;
    }
    @name(".dst_vtep_hit") action dst_vtep_hit(switch_tunnel_mode_t ttl_mode, switch_tunnel_mode_t qos_mode) {
        local_md.tunnel.ttl_mode = ttl_mode;
        local_md.tunnel.qos_mode = qos_mode;
    }
    @name(".set_inner_bd_properties_base") action set_inner_bd_properties_base(switch_bd_t bd, switch_vrf_t vrf, switch_packet_action_t vrf_ttl_violation, bool vrf_ttl_violation_valid, switch_packet_action_t vrf_ip_options_violation, switch_bd_label_t bd_label, switch_learning_mode_t learning_mode, bool ipv4_unicast_enable, bool ipv6_unicast_enable) {
        local_md.bd = bd;
        local_md.vrf = vrf;
        local_md.flags.vrf_ttl_violation = vrf_ttl_violation;
        local_md.flags.vrf_ttl_violation_valid = vrf_ttl_violation_valid;
        local_md.flags.vrf_ip_options_violation = vrf_ip_options_violation;
        local_md.learning.bd_mode = learning_mode;
        local_md.ipv4.unicast_enable = ipv4_unicast_enable;
        local_md.ipv4.multicast_enable = false;
        local_md.ipv4.multicast_snooping = false;
        local_md.ipv6.unicast_enable = ipv6_unicast_enable;
        local_md.ipv6.multicast_enable = false;
        local_md.ipv6.multicast_snooping = false;
        local_md.tunnel.terminate = true;
    }
    @name(".set_inner_bd_properties") action set_inner_bd_properties(switch_bd_t bd, switch_vrf_t vrf, switch_packet_action_t vrf_ttl_violation, bool vrf_ttl_violation_valid, switch_packet_action_t vrf_ip_options_violation, switch_bd_label_t bd_label, switch_learning_mode_t learning_mode, bool ipv4_unicast_enable, bool ipv6_unicast_enable, switch_tunnel_mode_t ttl_mode, switch_tunnel_mode_t qos_mode, switch_tunnel_mode_t ecn_mode) {
        set_inner_bd_properties_base(bd, vrf, vrf_ttl_violation, vrf_ttl_violation_valid, vrf_ip_options_violation, bd_label, learning_mode, ipv4_unicast_enable, ipv6_unicast_enable);
        local_md.tunnel.ttl_mode = ttl_mode;
        local_md.tunnel.qos_mode = qos_mode;
    }
    @name(".dst_vtep") table dst_vtep {
        key = {
            hdr.ipv4.src_addr   : ternary @name("src_addr");
            hdr.ipv4.dst_addr   : ternary @name("dst_addr");
            local_md.vrf        : exact;
            local_md.tunnel.type: exact;
        }
        actions = {
            NoAction;
            dst_vtep_hit;
            set_inner_bd_properties;
        }
        size = IPV4_DST_VTEP_TABLE_SIZE;
        const default_action = NoAction;
        requires_versioning = false;
    }
    @name(".dst_vtepv6") table dst_vtepv6 {
        key = {
            hdr.ipv6.src_addr   : ternary @name("src_addr");
            hdr.ipv6.dst_addr   : ternary @name("dst_addr");
            local_md.vrf        : exact;
            local_md.tunnel.type: exact;
        }
        actions = {
            NoAction;
            dst_vtep_hit;
            set_inner_bd_properties;
        }
        size = IPV6_DST_VTEP_TABLE_SIZE;
        const default_action = NoAction;
        requires_versioning = false;
    }
    action set_ip_tos_outer_ipv4() {
        lkp.ip_tos = hdr.ipv4.diffserv;
    }
    action set_ip_tos_outer_ipv6() {
        lkp.ip_tos = hdr.ipv6.traffic_class;
    }
    table lkp_ip_tos_outer {
        key = {
            hdr.ipv4.isValid(): exact;
            hdr.ipv6.isValid(): exact;
        }
        actions = {
            NoAction;
            set_ip_tos_outer_ipv4;
            set_ip_tos_outer_ipv6;
        }
        size = 4;
        const entries = {
                        (true, false) : set_ip_tos_outer_ipv4;
                        (false, true) : set_ip_tos_outer_ipv6;
        }
        const default_action = NoAction;
    }
    @name(".vni_to_bd_mapping") table vni_to_bd_mapping {
        key = {
            local_md.tunnel.vni: exact;
        }
        actions = {
            NoAction;
            set_inner_bd_properties_base;
        }
        default_action = NoAction;
        size = VNI_MAPPING_TABLE_SIZE;
    }
    apply {
        if (local_md.flags.rmac_hit) {
            if (lkp.ip_type == SWITCH_IP_TYPE_IPV4) {
                switch (dst_vtep.apply().action_run) {
                    dst_vtep_hit: {
                        if (vni_to_bd_mapping.apply().hit) {
                            if (!vxlan_rmac.apply().hit) {
                                vxlan_device_rmac.apply();
                            }
                            ;
                            pkt_validation.apply(hdr, local_md);
                        }
                    }
                    set_inner_bd_properties: {
                        pkt_validation.apply(hdr, local_md);
                    }
                }
            } else if (lkp.ip_type == SWITCH_IP_TYPE_IPV6) {
                switch (dst_vtepv6.apply().action_run) {
                    dst_vtep_hit: {
                        if (vni_to_bd_mapping.apply().hit) {
                            if (!vxlan_rmac.apply().hit) {
                                vxlan_device_rmac.apply();
                            }
                            ;
                            pkt_validation.apply(hdr, local_md);
                        }
                    }
                    set_inner_bd_properties: {
                        pkt_validation.apply(hdr, local_md);
                    }
                }
            }
        }
        if (local_md.tunnel.terminate && local_md.tunnel.qos_mode == SWITCH_TUNNEL_MODE_UNIFORM) {
            lkp_ip_tos_outer.apply();
        }
    }
}

control TunnelDecap(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    action store_outer_ipv4_fields() {
        local_md.tunnel.decap_tos = hdr.ipv4.diffserv;
        local_md.tunnel.decap_ttl = hdr.ipv4.ttl;
    }
    action store_outer_ipv6_fields() {
        local_md.tunnel.decap_tos = hdr.ipv6.traffic_class;
        local_md.tunnel.decap_ttl = hdr.ipv6.hop_limit;
    }
    action store_outer_mpls_fields() {
        local_md.tunnel.decap_exp = hdr.mpls[0].exp;
        local_md.tunnel.decap_ttl = hdr.mpls[0].ttl;
    }
    action copy_ipv4_header() {
        hdr.ipv4.setValid();
        hdr.ipv6.setInvalid();
        hdr.ipv4.version = hdr.inner_ipv4.version;
        hdr.ipv4.ihl = hdr.inner_ipv4.ihl;
        hdr.ipv4.total_len = hdr.inner_ipv4.total_len;
        hdr.ipv4.identification = hdr.inner_ipv4.identification;
        hdr.ipv4.flags = hdr.inner_ipv4.flags;
        hdr.ipv4.frag_offset = hdr.inner_ipv4.frag_offset;
        hdr.ipv4.protocol = hdr.inner_ipv4.protocol;
        hdr.ipv4.src_addr = hdr.inner_ipv4.src_addr;
        hdr.ipv4.dst_addr = hdr.inner_ipv4.dst_addr;
        hdr.ipv4.diffserv = hdr.inner_ipv4.diffserv;
        hdr.ipv4.ttl = hdr.inner_ipv4.ttl;
        hdr.inner_ipv4.setInvalid();
    }
    action copy_ipv6_header() {
        hdr.ipv6.setValid();
        hdr.ipv4.setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
    }
    action invalidate_vxlan_header() {
        hdr.vxlan.setInvalid();
        hdr.udp.setInvalid();
        hdr.inner_ethernet.setInvalid();
    }
    action invalidate_gre_header() {
    }
    action invalidate_vlan_tag0() {
        hdr.vlan_tag[0].setInvalid();
    }
    action decap_v4_inner_ethernet_ipv4() {
        store_outer_ipv4_fields();
        invalidate_vlan_tag0();
        hdr.ethernet = hdr.inner_ethernet;
        copy_ipv4_header();
        invalidate_vxlan_header();
    }
    action decap_v4_inner_ethernet_ipv6() {
        store_outer_ipv4_fields();
        invalidate_vlan_tag0();
        hdr.ethernet = hdr.inner_ethernet;
        copy_ipv6_header();
        invalidate_vxlan_header();
    }
    action decap_v4_inner_ethernet_non_ip() {
        store_outer_ipv4_fields();
        invalidate_vlan_tag0();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
        invalidate_vxlan_header();
    }
    action decap_v4_inner_ipv4() {
        store_outer_ipv4_fields();
        invalidate_vlan_tag0();
        hdr.ethernet.ether_type = 0x800;
        copy_ipv4_header();
        invalidate_gre_header();
    }
    action decap_v4_inner_ipv6() {
        store_outer_ipv4_fields();
        invalidate_vlan_tag0();
        hdr.ethernet.ether_type = 0x86dd;
        copy_ipv6_header();
        invalidate_gre_header();
    }
    action decap_v6_inner_ethernet_ipv4() {
        store_outer_ipv6_fields();
        invalidate_vlan_tag0();
        hdr.ethernet = hdr.inner_ethernet;
        copy_ipv4_header();
        invalidate_vxlan_header();
    }
    action decap_v6_inner_ethernet_ipv6() {
        store_outer_ipv6_fields();
        invalidate_vlan_tag0();
        hdr.ethernet = hdr.inner_ethernet;
        copy_ipv6_header();
        invalidate_vxlan_header();
    }
    action decap_v6_inner_ethernet_non_ip() {
        store_outer_ipv6_fields();
        invalidate_vlan_tag0();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
        invalidate_vxlan_header();
    }
    action decap_v6_inner_ipv4() {
        store_outer_ipv6_fields();
        invalidate_vlan_tag0();
        hdr.ethernet.ether_type = 0x800;
        copy_ipv4_header();
        invalidate_gre_header();
    }
    action decap_v6_inner_ipv6() {
        store_outer_ipv6_fields();
        invalidate_vlan_tag0();
        hdr.ethernet.ether_type = 0x86dd;
        copy_ipv6_header();
        invalidate_gre_header();
    }
    action decap_mpls_inner_ipv4() {
        store_outer_mpls_fields();
        invalidate_vlan_tag0();
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet.ether_type = 0x800;
        copy_ipv4_header();
    }
    action decap_mpls_inner_ipv6() {
        store_outer_mpls_fields();
        invalidate_vlan_tag0();
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet.ether_type = 0x86dd;
        copy_ipv6_header();
    }
    action mpls_pop1() {
        store_outer_mpls_fields();
        invalidate_vlan_tag0();
        hdr.mpls.pop_front(1);
    }
    action mpls_pop2() {
        store_outer_mpls_fields();
        invalidate_vlan_tag0();
        hdr.mpls.pop_front(2);
    }
    table decap_tunnel_hdr {
        key = {
            hdr.ipv4.isValid()          : exact;
            hdr.ipv6.isValid()          : exact;
            hdr.udp.isValid()           : exact;
            hdr.inner_ethernet.isValid(): exact;
            hdr.inner_ipv4.isValid()    : exact;
            hdr.inner_ipv6.isValid()    : exact;
        }
        actions = {
            decap_v4_inner_ethernet_ipv4;
            decap_v4_inner_ethernet_ipv6;
            decap_v4_inner_ethernet_non_ip;
            decap_v6_inner_ethernet_ipv4;
            decap_v6_inner_ethernet_ipv6;
            decap_v6_inner_ethernet_non_ip;
            decap_v4_inner_ipv4;
            decap_v4_inner_ipv6;
            decap_v6_inner_ipv4;
            decap_v6_inner_ipv6;
        }
        const entries = {
                        (true, false, true, true, true, false) : decap_v4_inner_ethernet_ipv4;
                        (true, false, true, true, false, true) : decap_v4_inner_ethernet_ipv6;
                        (true, false, true, true, false, false) : decap_v4_inner_ethernet_non_ip;
                        (false, true, true, true, true, false) : decap_v6_inner_ethernet_ipv4;
                        (false, true, true, true, false, true) : decap_v6_inner_ethernet_ipv6;
                        (false, true, true, true, false, false) : decap_v6_inner_ethernet_non_ip;
                        (true, false, false, false, true, false) : decap_v4_inner_ipv4;
                        (true, false, false, false, false, true) : decap_v4_inner_ipv6;
                        (false, true, false, false, true, false) : decap_v6_inner_ipv4;
                        (false, true, false, false, false, true) : decap_v6_inner_ipv6;
        }
        size = MIN_TABLE_SIZE;
    }
    action decap_len_outer_ipv4_udp() {
        local_md.pkt_length = local_md.pkt_length - hdr.ipv4.minSizeInBytes() - hdr.udp.minSizeInBytes() - hdr.vxlan.minSizeInBytes() - hdr.inner_ethernet.minSizeInBytes();
    }
    action decap_len_outer_ipv6_udp() {
        local_md.pkt_length = local_md.pkt_length - hdr.ipv6.minSizeInBytes() - hdr.udp.minSizeInBytes() - hdr.vxlan.minSizeInBytes() - hdr.inner_ethernet.minSizeInBytes();
    }
    action decap_len_outer_ipv4() {
        local_md.pkt_length = local_md.pkt_length - hdr.ipv4.minSizeInBytes();
    }
    action decap_len_outer_ipv6() {
        local_md.pkt_length = local_md.pkt_length - hdr.ipv6.minSizeInBytes();
    }
    action decap_len_outer_qtag_ipv4_udp() {
        local_md.pkt_length = local_md.pkt_length - hdr.ipv4.minSizeInBytes() - hdr.vlan_tag[0].minSizeInBytes() - hdr.udp.minSizeInBytes() - hdr.vxlan.minSizeInBytes() - hdr.inner_ethernet.minSizeInBytes();
    }
    action decap_len_outer_qtag_ipv6_udp() {
        local_md.pkt_length = local_md.pkt_length - hdr.ipv6.minSizeInBytes() - hdr.vlan_tag[0].minSizeInBytes() - hdr.udp.minSizeInBytes() - hdr.vxlan.minSizeInBytes() - hdr.inner_ethernet.minSizeInBytes();
    }
    action decap_len_outer_qtag_ipv4() {
        local_md.pkt_length = local_md.pkt_length - hdr.ipv4.minSizeInBytes() - hdr.vlan_tag[0].minSizeInBytes();
    }
    action decap_len_outer_qtag_ipv6() {
        local_md.pkt_length = local_md.pkt_length - hdr.ipv6.minSizeInBytes() - hdr.vlan_tag[0].minSizeInBytes();
    }
    action decap_len_pop1() {
        local_md.pkt_length = local_md.pkt_length - hdr.mpls[0].minSizeInBytes();
    }
    action decap_len_pop2() {
        local_md.pkt_length = local_md.pkt_length - 2 * hdr.mpls[0].minSizeInBytes();
    }
    action decap_len_pop3() {
        local_md.pkt_length = local_md.pkt_length - 3 * hdr.mpls[0].minSizeInBytes();
    }
    action decap_len_qtag_pop1() {
        local_md.pkt_length = local_md.pkt_length - hdr.mpls[0].minSizeInBytes() - hdr.vlan_tag[0].minSizeInBytes();
    }
    action decap_len_qtag_pop2() {
        local_md.pkt_length = local_md.pkt_length - 2 * hdr.mpls[0].minSizeInBytes() - hdr.vlan_tag[0].minSizeInBytes();
    }
    action decap_len_qtag_pop3() {
        local_md.pkt_length = local_md.pkt_length - 3 * hdr.mpls[0].minSizeInBytes() - hdr.vlan_tag[0].minSizeInBytes();
    }
    table decap_len_adjust {
        key = {
            hdr.udp.isValid()        : exact;
            hdr.ipv4.isValid()       : exact;
            hdr.ipv6.isValid()       : exact;
            hdr.vlan_tag[0].isValid(): exact;
        }
        actions = {
            decap_len_outer_ipv4_udp;
            decap_len_outer_ipv6_udp;
            decap_len_outer_ipv6;
            decap_len_outer_ipv4;
            decap_len_outer_qtag_ipv4_udp;
            decap_len_outer_qtag_ipv6_udp;
            decap_len_outer_qtag_ipv6;
            decap_len_outer_qtag_ipv4;
        }
        const entries = {
                        (true, true, false, false) : decap_len_outer_ipv4_udp;
                        (true, false, true, false) : decap_len_outer_ipv6_udp;
                        (false, true, false, false) : decap_len_outer_ipv4;
                        (false, false, true, false) : decap_len_outer_ipv6;
                        (true, true, false, true) : decap_len_outer_qtag_ipv4_udp;
                        (true, false, true, true) : decap_len_outer_qtag_ipv6_udp;
                        (false, true, false, true) : decap_len_outer_qtag_ipv4;
                        (false, false, true, true) : decap_len_outer_qtag_ipv6;
        }
        size = MIN_TABLE_SIZE;
    }
    action decap_ttl_inner_v4_uniform() {
        hdr.ipv4.ttl = local_md.tunnel.decap_ttl;
    }
    action decap_ttl_inner_v6_uniform() {
        hdr.ipv6.hop_limit = local_md.tunnel.decap_ttl;
    }
    table decap_ttl {
        key = {
            hdr.ipv4.isValid()      : exact;
            hdr.ipv6.isValid()      : exact;
            local_md.tunnel.ttl_mode: exact;
        }
        actions = {
            NoAction;
            decap_ttl_inner_v4_uniform;
            decap_ttl_inner_v6_uniform;
        }
        const entries = {
                        (true, false, SWITCH_TUNNEL_MODE_UNIFORM) : decap_ttl_inner_v4_uniform;
                        (false, true, SWITCH_TUNNEL_MODE_UNIFORM) : decap_ttl_inner_v6_uniform;
        }
        const default_action = NoAction;
        size = 32;
    }
    action decap_dscp_inner_v4_uniform() {
        @in_hash {
            hdr.ipv4.diffserv = local_md.tunnel.decap_tos;
        }
    }
    action decap_dscp_inner_v6_uniform() {
        @in_hash {
            hdr.ipv6.traffic_class = local_md.tunnel.decap_tos;
        }
    }
    action decap_ecn_inner_v4_from_outer() {
        @in_hash {
            hdr.ipv4.diffserv[1:0] = local_md.tunnel.decap_tos[1:0];
        }
    }
    action decap_ecn_inner_v6_from_outer() {
        @in_hash {
            hdr.ipv6.traffic_class[1:0] = local_md.tunnel.decap_tos[1:0];
        }
    }
    table decap_dscp {
        key = {
            hdr.ipv4.isValid()      : exact;
            hdr.ipv6.isValid()      : exact;
            local_md.tunnel.qos_mode: exact;
        }
        actions = {
            NoAction;
            decap_dscp_inner_v4_uniform;
            decap_dscp_inner_v6_uniform;
            decap_ecn_inner_v4_from_outer;
            decap_ecn_inner_v6_from_outer;
        }
        const entries = {
                        (true, false, SWITCH_TUNNEL_MODE_UNIFORM) : decap_dscp_inner_v4_uniform;
                        (false, true, SWITCH_TUNNEL_MODE_UNIFORM) : decap_dscp_inner_v6_uniform;
                        (true, false, SWITCH_TUNNEL_MODE_PIPE) : decap_ecn_inner_v4_from_outer;
                        (false, true, SWITCH_TUNNEL_MODE_PIPE) : decap_ecn_inner_v6_from_outer;
        }
        size = 32;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            decap_len_adjust.apply();
            decap_tunnel_hdr.apply();
            decap_ttl.apply();
            decap_dscp.apply();
        }
    }
}

control TunnelNexthop(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".l2_tunnel_encap") action l2_tunnel_encap(switch_tunnel_type_t type, switch_tunnel_ip_index_t dip_index, switch_tunnel_index_t tunnel_index, switch_tunnel_mapper_index_t tunnel_mapper_index) {
        local_md.tunnel.type = type;
        local_md.tunnel.index = tunnel_index;
        local_md.tunnel.dip_index = dip_index;
        local_md.flags.routed = true;
        local_md.flags.l2_tunnel_encap = true;
    }
    @name(".l3_tunnel_encap") action l3_tunnel_encap(mac_addr_t dmac, switch_tunnel_type_t type, switch_tunnel_ip_index_t dip_index, switch_tunnel_index_t tunnel_index) {
        local_md.flags.routed = true;
        local_md.tunnel.type = type;
        local_md.tunnel.dip_index = dip_index;
        local_md.tunnel.index = tunnel_index;
        hdr.ethernet.dst_addr = dmac;
    }
    @name(".l3_tunnel_encap_with_vni") action l3_tunnel_encap_with_vni(mac_addr_t dmac, switch_tunnel_type_t type, switch_tunnel_vni_t vni, switch_tunnel_ip_index_t dip_index, switch_tunnel_index_t tunnel_index) {
        local_md.flags.routed = true;
        local_md.tunnel.type = type;
        local_md.tunnel.index = tunnel_index;
        local_md.tunnel.dip_index = dip_index;
        hdr.ethernet.dst_addr = dmac;
        local_md.tunnel.vni = vni;
    }
    @name(".srv6_encap") action srv6_encap(switch_tunnel_index_t tunnel_index, bit<3> seg_len) {
        local_md.flags.routed = true;
        local_md.tunnel.type = SWITCH_EGRESS_TUNNEL_TYPE_SRV6_ENCAP;
        local_md.tunnel.index = tunnel_index;
        local_md.tunnel.srv6_seg_len = seg_len;
    }
    @name(".srv6_insert") action srv6_insert(bit<3> seg_len) {
        local_md.flags.routed = true;
        local_md.tunnel.type = SWITCH_EGRESS_TUNNEL_TYPE_SRV6_INSERT;
        local_md.tunnel.srv6_seg_len = seg_len;
    }
    @name(".tunnel_nexthop") table tunnel_nexthop {
        key = {
            local_md.tunnel_nexthop: exact;
        }
        actions = {
            NoAction;
            l2_tunnel_encap;
            l3_tunnel_encap;
            l3_tunnel_encap_with_vni;
        }
        const default_action = NoAction;
        size = TUNNEL_NEXTHOP_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            tunnel_nexthop.apply();
        }
    }
}

control TunnelEncap(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    bit<16> payload_len;
    bit<8> ip_proto;
    bit<16> gre_proto;
    action copy_ipv4_header() {
        hdr.inner_ipv4.setValid();
        hdr.inner_ipv6.setInvalid();
        hdr.inner_ipv4.version = hdr.ipv4.version;
        hdr.inner_ipv4.ihl = hdr.ipv4.ihl;
        hdr.inner_ipv4.diffserv = hdr.ipv4.diffserv;
        hdr.inner_ipv4.total_len = hdr.ipv4.total_len;
        hdr.inner_ipv4.identification = hdr.ipv4.identification;
        hdr.inner_ipv4.flags = hdr.ipv4.flags;
        hdr.inner_ipv4.frag_offset = hdr.ipv4.frag_offset;
        hdr.inner_ipv4.ttl = hdr.ipv4.ttl;
        hdr.inner_ipv4.protocol = hdr.ipv4.protocol;
        hdr.inner_ipv4.src_addr = hdr.ipv4.src_addr;
        hdr.inner_ipv4.dst_addr = hdr.ipv4.dst_addr;
        local_md.inner_ipv4_checksum_update_en = true;
        hdr.ipv4.setInvalid();
    }
    action copy_inner_ipv4_udp() {
        payload_len = hdr.ipv4.total_len;
        copy_ipv4_header();
        hdr.inner_udp = hdr.udp;
        hdr.udp.setInvalid();
        hdr.inner_udp.setValid();
        ip_proto = 4;
        gre_proto = 0x800;
    }
    action copy_inner_ipv4_unknown() {
        payload_len = hdr.ipv4.total_len;
        copy_ipv4_header();
        ip_proto = 4;
        gre_proto = 0x800;
    }
    action copy_inner_ipv6_udp() {
        payload_len = hdr.ipv6.payload_len + 16w40;
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.ipv6.setInvalid();
        hdr.inner_ipv4.setInvalid();
        hdr.inner_udp = hdr.udp;
        ip_proto = 41;
        gre_proto = 0x86dd;
        hdr.udp.setInvalid();
    }
    action copy_inner_ipv6_unknown() {
        payload_len = hdr.ipv6.payload_len + 16w40;
        hdr.inner_ipv6 = hdr.ipv6;
        ip_proto = 41;
        gre_proto = 0x86dd;
        hdr.ipv6.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    action copy_inner_non_ip() {
        payload_len = local_md.pkt_length - 16w14;
    }
    table tunnel_encap_0 {
        key = {
            hdr.ipv4.isValid(): exact;
            hdr.ipv6.isValid(): exact;
            hdr.udp.isValid() : exact;
        }
        actions = {
            copy_inner_ipv4_udp;
            copy_inner_ipv4_unknown;
            copy_inner_ipv6_udp;
            copy_inner_ipv6_unknown;
            copy_inner_non_ip;
        }
        const entries = {
                        (true, false, false) : copy_inner_ipv4_unknown();
                        (false, true, false) : copy_inner_ipv6_unknown();
                        (true, false, true) : copy_inner_ipv4_udp();
                        (false, true, true) : copy_inner_ipv6_udp();
                        (false, false, false) : copy_inner_non_ip();
        }
        size = 8;
    }
    action add_udp_header(bit<16> src_port, bit<16> dst_port) {
        hdr.udp.setValid();
        hdr.udp.src_port = src_port;
        hdr.udp.dst_port = dst_port;
        local_md.lkp.l4_src_port = src_port;
        local_md.lkp.l4_dst_port = dst_port;
        hdr.udp.checksum = 0;
    }
    action clear_l4_lkp_fields() {
    }
    action add_vxlan_header(bit<24> vni) {
        hdr.vxlan.setValid();
        hdr.vxlan.flags = 8w0x8;
        hdr.vxlan.vni = vni;
    }
    action add_gre_header(bit<16> proto) {
    }
    action add_ipv4_header(bit<8> proto) {
        hdr.ipv4.setValid();
        hdr.ipv4.version = 4w4;
        hdr.ipv4.ihl = 4w5;
        hdr.ipv4.identification = 0;
        hdr.ipv4.flags = 0;
        hdr.ipv4.frag_offset = 0;
        hdr.ipv4.protocol = proto;
        hdr.ipv6.setInvalid();
        local_md.lkp.ip_proto = hdr.ipv4.protocol;
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
    }
    action add_ipv6_header(bit<8> proto) {
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w6;
        hdr.ipv6.flow_label = 0;
        hdr.ipv6.next_hdr = proto;
        hdr.ipv4.setInvalid();
        local_md.lkp.ip_proto = hdr.ipv6.next_hdr;
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
    }
    @name(".encap_ipv4_vxlan") action encap_ipv4_vxlan(bit<16> vxlan_port) {
        hdr.inner_ethernet = hdr.ethernet;
        add_ipv4_header(17);
        hdr.ipv4.flags = 0x2;
        hdr.ipv4.total_len = payload_len + hdr.ipv4.minSizeInBytes() + hdr.udp.minSizeInBytes() + hdr.vxlan.minSizeInBytes() + hdr.inner_ethernet.minSizeInBytes();
        add_udp_header(local_md.tunnel.hash, vxlan_port);
        hdr.udp.length = payload_len + hdr.udp.minSizeInBytes() + hdr.vxlan.minSizeInBytes() + hdr.inner_ethernet.minSizeInBytes();
        local_md.pkt_length = local_md.pkt_length + hdr.ipv4.minSizeInBytes() + hdr.udp.minSizeInBytes() + hdr.vxlan.minSizeInBytes() + hdr.inner_ethernet.minSizeInBytes();
        add_vxlan_header(local_md.tunnel.vni);
        hdr.ethernet.ether_type = 0x800;
    }
    @name(".encap_ipv6_vxlan") action encap_ipv6_vxlan(bit<16> vxlan_port) {
        hdr.inner_ethernet = hdr.ethernet;
        add_ipv6_header(17);
        hdr.ipv6.payload_len = payload_len + hdr.udp.minSizeInBytes() + hdr.vxlan.minSizeInBytes() + hdr.inner_ethernet.minSizeInBytes();
        add_udp_header(local_md.tunnel.hash, vxlan_port);
        hdr.udp.length = payload_len + hdr.udp.minSizeInBytes() + hdr.vxlan.minSizeInBytes() + hdr.inner_ethernet.minSizeInBytes();
        local_md.pkt_length = local_md.pkt_length + hdr.ipv6.minSizeInBytes() + hdr.udp.minSizeInBytes() + hdr.vxlan.minSizeInBytes() + hdr.inner_ethernet.minSizeInBytes();
        add_vxlan_header(local_md.tunnel.vni);
        hdr.ethernet.ether_type = 0x86dd;
    }
    @name(".encap_ipv4_ip") action encap_ipv4_ip() {
        add_ipv4_header(ip_proto);
        hdr.ipv4.total_len = payload_len + hdr.ipv4.minSizeInBytes();
        local_md.pkt_length = local_md.pkt_length + hdr.ipv4.minSizeInBytes();
        hdr.ethernet.ether_type = 0x800;
        clear_l4_lkp_fields();
    }
    @name(".encap_ipv6_ip") action encap_ipv6_ip() {
        add_ipv6_header(ip_proto);
        hdr.ipv6.payload_len = payload_len;
        local_md.pkt_length = local_md.pkt_length + hdr.ipv6.minSizeInBytes();
        hdr.ethernet.ether_type = 0x86dd;
        clear_l4_lkp_fields();
    }
    @name(".tunnel_encap_1") table tunnel_encap_1 {
        key = {
            local_md.tunnel.type: exact;
        }
        actions = {
            NoAction;
            encap_ipv4_vxlan;
            encap_ipv6_vxlan;
            encap_ipv4_ip;
            encap_ipv6_ip;
        }
        const default_action = NoAction;
        size = MIN_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress && local_md.tunnel.type != SWITCH_EGRESS_TUNNEL_TYPE_NONE && local_md.tunnel_nexthop != 0) {
            if (local_md.tunnel.type != SWITCH_EGRESS_TUNNEL_TYPE_SRV6_INSERT) {
                tunnel_encap_0.apply();
            }
            if (local_md.tunnel.type == SWITCH_EGRESS_TUNNEL_TYPE_MPLS) {
            } else {
                tunnel_encap_1.apply();
            }
        }
    }
}

control TunnelRewrite(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".ipv4_sip_rewrite") action ipv4_sip_rewrite(ipv4_addr_t src_addr, bit<8> ttl_val, bit<6> dscp_val) {
        hdr.ipv4.src_addr = src_addr;
        hdr.ipv4.diffserv[7:2] = dscp_val;
        local_md.lkp.ip_tos[7:2] = dscp_val;
    }
    @name(".ipv6_sip_rewrite") action ipv6_sip_rewrite(ipv6_addr_t src_addr, bit<8> ttl_val, bit<6> dscp_val) {
        hdr.ipv6.src_addr = src_addr;
        hdr.ipv6.traffic_class[7:2] = dscp_val;
        local_md.lkp.ip_tos[7:2] = dscp_val;
    }
    @name(".src_addr_rewrite") table src_addr_rewrite {
        key = {
            local_md.tunnel.index: exact;
        }
        actions = {
            ipv4_sip_rewrite;
            ipv6_sip_rewrite;
        }
        size = TUNNEL_OBJECT_SIZE;
    }
    @name(".encap_ttl_v4_in_v4_pipe") action encap_ttl_v4_in_v4_pipe(bit<8> ttl_val) {
        hdr.ipv4.ttl = ttl_val;
    }
    @name(".encap_ttl_v4_in_v6_pipe") action encap_ttl_v4_in_v6_pipe(bit<8> ttl_val) {
        hdr.ipv6.hop_limit = ttl_val;
    }
    @name(".encap_ttl_v6_in_v4_pipe") action encap_ttl_v6_in_v4_pipe(bit<8> ttl_val) {
        hdr.ipv4.ttl = ttl_val;
    }
    @name(".encap_ttl_v6_in_v6_pipe") action encap_ttl_v6_in_v6_pipe(bit<8> ttl_val) {
        hdr.ipv6.hop_limit = ttl_val;
    }
    @name(".encap_ttl_v4_in_v4_uniform") action encap_ttl_v4_in_v4_uniform() {
        hdr.ipv4.ttl = hdr.inner_ipv4.ttl;
    }
    @name(".encap_ttl_v4_in_v6_uniform") action encap_ttl_v4_in_v6_uniform() {
        hdr.ipv6.hop_limit = hdr.inner_ipv4.ttl;
    }
    @name(".encap_ttl_v6_in_v4_uniform") action encap_ttl_v6_in_v4_uniform() {
        hdr.ipv4.ttl = hdr.inner_ipv6.hop_limit;
    }
    @name(".encap_ttl_v6_in_v6_uniform") action encap_ttl_v6_in_v6_uniform() {
        hdr.ipv6.hop_limit = hdr.inner_ipv6.hop_limit;
    }
    @name(".tunnel_rewrite_encap_ttl") table encap_ttl {
        key = {
            hdr.inner_ipv4.isValid(): exact;
            hdr.inner_ipv6.isValid(): exact;
            hdr.ipv4.isValid()      : exact;
            hdr.ipv6.isValid()      : exact;
            local_md.tunnel.index   : exact;
        }
        actions = {
            NoAction;
            encap_ttl_v4_in_v4_pipe;
            encap_ttl_v4_in_v6_pipe;
            encap_ttl_v6_in_v4_pipe;
            encap_ttl_v6_in_v6_pipe;
            encap_ttl_v4_in_v4_uniform;
            encap_ttl_v4_in_v6_uniform;
            encap_ttl_v6_in_v4_uniform;
            encap_ttl_v6_in_v6_uniform;
        }
        const default_action = NoAction;
        size = TUNNEL_OBJECT_SIZE * 3;
    }
    @name(".encap_dscp_v4_in_v4_ecn") action encap_dscp_v4_in_v4_ecn() {
    }
    @name(".encap_dscp_v4_in_v6_ecn") action encap_dscp_v4_in_v6_ecn() {
        @in_hash {
            hdr.ipv6.traffic_class[1:0] = hdr.inner_ipv4.diffserv[1:0];
        }
    }
    @name(".encap_dscp_v6_in_v4_ecn") action encap_dscp_v6_in_v4_ecn() {
        @in_hash {
            hdr.ipv4.diffserv[1:0] = hdr.inner_ipv6.traffic_class[1:0];
        }
    }
    @name(".encap_dscp_v6_in_v6_ecn") action encap_dscp_v6_in_v6_ecn() {
    }
    @name(".encap_dscp_v4_in_v4_uniform") action encap_dscp_v4_in_v4_uniform() {
        hdr.ipv4.diffserv = hdr.inner_ipv4.diffserv;
        local_md.lkp.ip_tos = hdr.inner_ipv4.diffserv;
    }
    @name(".encap_dscp_v4_in_v6_uniform") action encap_dscp_v4_in_v6_uniform() {
        @in_hash {
            hdr.ipv6.traffic_class = hdr.inner_ipv4.diffserv;
        }
        @in_hash {
            local_md.lkp.ip_tos = hdr.inner_ipv4.diffserv;
        }
    }
    @name(".encap_dscp_v6_in_v4_uniform") action encap_dscp_v6_in_v4_uniform() {
        @in_hash {
            hdr.ipv4.diffserv = hdr.inner_ipv6.traffic_class;
        }
        @in_hash {
            local_md.lkp.ip_tos = hdr.inner_ipv6.traffic_class;
        }
    }
    @name(".encap_dscp_v6_in_v6_uniform") action encap_dscp_v6_in_v6_uniform() {
        hdr.ipv6.traffic_class = hdr.inner_ipv6.traffic_class;
        local_md.lkp.ip_tos = hdr.inner_ipv6.traffic_class;
    }
    @name(".tunnel_rewrite_encap_dscp") table encap_dscp {
        key = {
            hdr.inner_ipv4.isValid(): exact;
            hdr.inner_ipv6.isValid(): exact;
            hdr.ipv4.isValid()      : exact;
            hdr.ipv6.isValid()      : exact;
            local_md.tunnel.index   : exact;
        }
        actions = {
            NoAction;
            encap_dscp_v4_in_v4_ecn;
            encap_dscp_v4_in_v6_ecn;
            encap_dscp_v6_in_v4_ecn;
            encap_dscp_v6_in_v6_ecn;
            encap_dscp_v4_in_v4_uniform;
            encap_dscp_v4_in_v6_uniform;
            encap_dscp_v6_in_v4_uniform;
            encap_dscp_v6_in_v6_uniform;
        }
        const default_action = NoAction;
        size = TUNNEL_OBJECT_SIZE * 3;
    }
    @name(".ipv4_dip_rewrite") action ipv4_dip_rewrite(ipv4_addr_t dst_addr) {
        hdr.ipv4.dst_addr = dst_addr;
    }
    @name(".ipv6_dip_rewrite") action ipv6_dip_rewrite(ipv6_addr_t dst_addr) {
        hdr.ipv6.dst_addr = dst_addr;
    }
    @name(".dst_addr_rewrite") table dst_addr_rewrite {
        key = {
            local_md.tunnel.dip_index: exact;
        }
        actions = {
            ipv4_dip_rewrite;
            ipv6_dip_rewrite;
        }
        const default_action = ipv4_dip_rewrite(0);
        size = TUNNEL_ENCAP_IP_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress && local_md.tunnel_nexthop != 0 && local_md.tunnel.type != SWITCH_EGRESS_TUNNEL_TYPE_NONE) {
            if (local_md.tunnel.type == SWITCH_EGRESS_TUNNEL_TYPE_MPLS) {
            } else {
                dst_addr_rewrite.apply();
            }
            if (local_md.tunnel.type != SWITCH_EGRESS_TUNNEL_TYPE_SRV6_INSERT && local_md.tunnel.type != SWITCH_EGRESS_TUNNEL_TYPE_MPLS) {
                src_addr_rewrite.apply();
                encap_ttl.apply();
                encap_dscp.apply();
            }
        } else {
        }
    }
}

control VniMap(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".set_vni") action set_vni(switch_tunnel_vni_t vni) {
        local_md.tunnel.vni = vni;
    }
    @name(".bd_to_vni_mapping") table bd_to_vni_mapping {
        key = {
            local_md.bd[11:0]: exact;
        }
        actions = {
            set_vni;
        }
        const default_action = set_vni(0);
        size = BD_TO_VNI_MAPPING_SIZE;
    }
    @name(".vrf_to_vni_mapping") @use_hash_action(1) table vrf_to_vni_mapping {
        key = {
            local_md.vrf: exact;
        }
        actions = {
            set_vni;
        }
        const default_action = set_vni(0);
        size = VRF_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress && local_md.tunnel.vni == 0) {
            if (local_md.flags.l2_tunnel_encap) {
            } else {
                vrf_to_vni_mapping.apply();
            }
        }
    }
}

control MulticastFlooding(inout switch_local_metadata_t local_md)(switch_uint32_t table_size) {
    @name(".mcast_flood") action flood(switch_mgid_t mgid) {
        local_md.multicast.id = mgid;
    }
    @name(".bd_flood") table bd_flood {
        key = {
            local_md.bd          : exact @name("bd");
            local_md.lkp.pkt_type: exact @name("pkt_type");
        }
        actions = {
            flood;
        }
        size = table_size;
    }
    apply {
        bd_flood.apply();
    }
}

control Replication(in egress_intrinsic_metadata_t eg_intr_md, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=4096) {
    @name(".mc_rid_hit") action rid_hit_mc(switch_bd_t bd) {
        local_md.checks.same_bd = bd ^ local_md.bd;
        local_md.bd = bd;
    }
    action rid_miss() {
        local_md.flags.routed = false;
    }
    @name(".rid") table rid {
        key = {
            eg_intr_md.egress_rid: exact;
        }
        actions = {
            rid_miss;
            rid_hit_mc;
        }
        size = table_size;
        const default_action = rid_miss;
    }
    apply {
        if (eg_intr_md.egress_rid != 0) {
            rid.apply();
        }
    }
}

control ECNAcl(in switch_local_metadata_t local_md, in switch_lookup_fields_t lkp, inout switch_pkt_color_t pkt_color)(switch_uint32_t table_size=512) {
    @name(".ecn_acl.set_ingress_color") action set_ingress_color(switch_pkt_color_t color) {
        pkt_color = color;
    }
    @name(".ecn_acl.acl") table acl {
        key = {
            local_md.ingress_port_lag_label: ternary;
            lkp.ip_tos                     : ternary;
            lkp.tcp_flags                  : ternary;
        }
        actions = {
            NoAction;
            set_ingress_color;
        }
        const default_action = NoAction;
        size = table_size;
    }
    apply {
        acl.apply();
    }
}

control IngressPFCWd(in switch_port_t port, in switch_qid_t qid, out bool flag)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".ingress_pfcwd.acl_deny") action acl_deny() {
        flag = true;
        stats.count();
    }
    @ways(2) @name(".ingress_pfcwd.acl") table acl {
        key = {
            qid : exact;
            port: exact;
        }
        actions = {
            @defaultonly NoAction;
            acl_deny;
        }
        const default_action = NoAction;
        counters = stats;
        size = table_size;
    }
    apply {
        acl.apply();
    }
}

control EgressPFCWd(in switch_port_t port, in switch_qid_t qid, out bool flag)(switch_uint32_t table_size=512) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".egress_pfcwd.acl_deny") action acl_deny() {
        flag = true;
        stats.count();
    }
    @ways(2) @name(".egress_pfcwd.acl") table acl {
        key = {
            qid : exact;
            port: exact;
        }
        actions = {
            @defaultonly NoAction;
            acl_deny;
        }
        const default_action = NoAction;
        counters = stats;
        size = table_size;
    }
    apply {
        acl.apply();
    }
}

control IngressQoSMap(inout switch_header_t hdr, inout switch_local_metadata_t local_md)(switch_uint32_t dscp_map_size=4096, switch_uint32_t pcp_map_size=512) {
    @name(".ingress_qos_map.set_ingress_tc") action set_ingress_tc(switch_tc_t tc) {
        local_md.qos.tc = tc;
    }
    @name(".ingress_qos_map.set_ingress_color") action set_ingress_color(switch_pkt_color_t color) {
        local_md.qos.color = color;
    }
    @name(".ingress_qos_map.set_ingress_tc_and_color") action set_ingress_tc_and_color(switch_tc_t tc, switch_pkt_color_t color) {
        set_ingress_tc(tc);
        set_ingress_color(color);
    }
    @name(".ingress_qos_map.dscp_tc_map") table dscp_tc_map {
        key = {
            local_md.ingress_port   : exact;
            local_md.lkp.ip_tos[7:2]: exact;
        }
        actions = {
            NoAction;
            set_ingress_tc;
            set_ingress_color;
            set_ingress_tc_and_color;
        }
        const default_action = NoAction;
        size = dscp_map_size;
    }
    @name(".ingress_qos_map.pcp_tc_map") table pcp_tc_map {
        key = {
            local_md.ingress_port: exact;
            local_md.lkp.pcp     : exact;
        }
        actions = {
            NoAction;
            set_ingress_tc;
            set_ingress_color;
            set_ingress_tc_and_color;
        }
        const default_action = NoAction;
        size = pcp_map_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_QOS != 0)) {
            if (local_md.lkp.ip_type != SWITCH_IP_TYPE_NONE) {
                switch (dscp_tc_map.apply().action_run) {
                    NoAction: {
                        pcp_tc_map.apply();
                    }
                }
            } else {
                pcp_tc_map.apply();
            }
        }
    }
}

control IngressTC(inout switch_local_metadata_t local_md) {
    const bit<32> tc_table_size = 1024;
    @name(".ingress_tc.set_icos") action set_icos(switch_cos_t icos) {
        local_md.qos.icos = icos;
    }
    @name(".ingress_tc.set_queue") action set_queue(switch_qid_t qid) {
        local_md.qos.qid = qid;
    }
    @name(".ingress_tc.set_icos_and_queue") action set_icos_and_queue(switch_cos_t icos, switch_qid_t qid) {
        set_icos(icos);
        set_queue(qid);
    }
    @name(".ingress_tc.traffic_class") table traffic_class {
        key = {
            local_md.ingress_port: ternary @name("port");
            local_md.qos.color   : ternary @name("color");
            local_md.qos.tc      : exact @name("tc");
        }
        actions = {
            set_icos;
            set_queue;
            set_icos_and_queue;
        }
        size = tc_table_size;
    }
    apply {
        if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_QOS != 0)) {
            traffic_class.apply();
        }
    }
}

control PPGStats(inout switch_local_metadata_t local_md) {
    const bit<32> ppg_table_size = 1024;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) ppg_stats;
    @name(".ppg_stats.count") action count() {
        ppg_stats.count();
    }
    @ways(2) @name(".ppg_stats.ppg") table ppg {
        key = {
            local_md.ingress_port: exact @name("port");
            local_md.qos.icos    : exact @name("icos");
        }
        actions = {
            @defaultonly NoAction;
            count;
        }
        const default_action = NoAction;
        size = ppg_table_size;
        counters = ppg_stats;
    }
    apply {
        ppg.apply();
    }
}

control EgressQoS(inout switch_header_t hdr, in switch_port_t port, inout switch_local_metadata_t local_md)(switch_uint32_t table_size=1024) {
    @name(".egress_qos.set_ipv4_dscp") action set_ipv4_dscp(bit<6> dscp, bit<3> exp) {
        hdr.ipv4.diffserv[7:2] = dscp;
        local_md.tunnel.mpls_encap_exp = exp;
    }
    @name(".egress_qos.set_ipv6_dscp") action set_ipv6_dscp(bit<6> dscp, bit<3> exp) {
        hdr.ipv6.traffic_class[7:2] = dscp;
        local_md.tunnel.mpls_encap_exp = exp;
    }
    @name(".egress_qos.set_vlan_pcp") action set_vlan_pcp(bit<3> pcp, bit<3> exp) {
        local_md.qos.pcp = pcp;
        local_md.tunnel.mpls_encap_exp = exp;
    }
    @name(".egress_qos.l3_qos_map") table l3_qos_map {
        key = {
            port              : exact @name("port");
            local_md.qos.tc   : exact @name("tc");
            local_md.qos.color: exact @name("color");
            hdr.ipv4.isValid(): exact;
            hdr.ipv6.isValid(): exact;
        }
        actions = {
            NoAction;
            set_ipv4_dscp;
            set_ipv6_dscp;
        }
        const default_action = NoAction;
        size = 12288;
    }
    @name(".egress_qos.l2_qos_map") table l2_qos_map {
        key = {
            port              : exact @name("port");
            local_md.qos.tc   : exact @name("tc");
            local_md.qos.color: exact @name("color");
        }
        actions = {
            NoAction;
            set_vlan_pcp;
        }
        const default_action = NoAction;
        size = 6144;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            l2_qos_map.apply();
            l3_qos_map.apply();
        }
    }
}

control EgressQueue(in switch_port_t port, inout switch_local_metadata_t local_md)(switch_uint32_t queue_table_size=1024) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) queue_stats;
    @name(".egress_queue.count") action count() {
        queue_stats.count();
    }
    @ways(2) @pack(2) @name(".egress_queue.queue") table queue {
        key = {
            port            : exact;
            local_md.qos.qid: exact @name("qid");
        }
        actions = {
            @defaultonly NoAction;
            count;
        }
        size = queue_table_size;
        const default_action = NoAction;
        counters = queue_stats;
    }
    apply {
        queue.apply();
    }
}

control StormControl(inout switch_local_metadata_t local_md, in switch_pkt_type_t pkt_type, out bool flag)(switch_uint32_t table_size=256, switch_uint32_t meter_size=1024) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) storm_control_stats;
    @name(".storm_control.meter") Meter<bit<16>>(meter_size, MeterType_t.BYTES) meter;
    @name(".storm_control.count") action count() {
        storm_control_stats.count();
        flag = false;
    }
    @name(".storm_control.drop_and_count") action drop_and_count() {
        storm_control_stats.count();
        flag = true;
    }
    @name(".storm_control.stats") table stats {
        key = {
            local_md.qos.storm_control_color: exact;
            pkt_type                        : ternary;
            local_md.ingress_port           : exact;
            local_md.flags.dmac_miss        : ternary;
        }
        actions = {
            @defaultonly NoAction;
            count;
            drop_and_count;
        }
        const default_action = NoAction;
        size = table_size * 2;
        counters = storm_control_stats;
    }
    @name(".storm_control.set_meter") action set_meter(bit<16> index) {
        local_md.qos.storm_control_color = (bit<2>)meter.execute(index);
    }
    @name(".storm_control.storm_control") table storm_control {
        key = {
            local_md.ingress_port   : exact;
            pkt_type                : ternary;
            local_md.flags.dmac_miss: ternary;
        }
        actions = {
            @defaultonly NoAction;
            set_meter;
        }
        const default_action = NoAction;
        size = table_size;
    }
    apply {
    }
}

control IngressMirrorMeter(inout switch_local_metadata_t local_md)(switch_uint32_t table_size=256) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".ingress_mirror_meter.meter") Meter<bit<9>>(table_size, MeterType_t.PACKETS) meter;
    switch_pkt_color_t color;
    @name(".ingress_mirror_meter.mirror_and_count") action mirror_and_count() {
        stats.count();
    }
    @name(".ingress_mirror_meter.no_mirror_and_count") action no_mirror_and_count() {
        stats.count();
        local_md.mirror.type = 0;
    }
    @ways(2) @name(".ingress_mirror_meter.meter_action") table meter_action {
        key = {
            color                      : exact;
            local_md.mirror.meter_index: exact;
        }
        actions = {
            @defaultonly NoAction;
            mirror_and_count;
            no_mirror_and_count;
        }
        const default_action = NoAction;
        size = table_size * 2;
        counters = stats;
    }
    @name(".ingress_mirror_meter.set_meter") action set_meter(bit<9> index) {
        color = (bit<2>)meter.execute(index);
    }
    @name(".ingress_mirror_meter.meter_index") table meter_index {
        key = {
            local_md.mirror.meter_index: exact;
        }
        actions = {
            @defaultonly NoAction;
            set_meter;
        }
        const default_action = NoAction;
        size = table_size;
    }
    apply {
        meter_index.apply();
        meter_action.apply();
    }
}

control EgressMirrorMeter(inout switch_local_metadata_t local_md)(switch_uint32_t table_size=256) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".egress_mirror_meter.meter") Meter<bit<9>>(table_size, MeterType_t.PACKETS) meter;
    switch_pkt_color_t color;
    @name(".egress_mirror_meter.mirror_and_count") action mirror_and_count() {
        stats.count();
    }
    @name(".egress_mirror_meter.no_mirror_and_count") action no_mirror_and_count() {
        stats.count();
        local_md.mirror.type = 0;
    }
    @ways(2) @name(".egress_mirror_meter.meter_action") table meter_action {
        key = {
            color                      : exact;
            local_md.mirror.meter_index: exact;
        }
        actions = {
            @defaultonly NoAction;
            mirror_and_count;
            no_mirror_and_count;
        }
        const default_action = NoAction;
        size = table_size * 2;
        counters = stats;
    }
    @name(".egress_mirror_meter.set_meter") action set_meter(bit<9> index) {
        color = (bit<2>)meter.execute(index);
    }
    @name(".egress_mirror_meter.meter_index") table meter_index {
        key = {
            local_md.mirror.meter_index: exact;
        }
        actions = {
            @defaultonly NoAction;
            set_meter;
        }
        const default_action = NoAction;
        size = table_size;
    }
    apply {
        meter_index.apply();
        meter_action.apply();
    }
}

control WRED(inout switch_header_t hdr, inout switch_local_metadata_t local_md, in egress_intrinsic_metadata_t eg_intr_md, out bool wred_drop) {
    switch_wred_index_t index;
    bit<1> wred_flag;
    const switch_uint32_t wred_size = 1 << 10;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) stats;
    @name(".egress_wred.wred") Wred<bit<19>, switch_wred_index_t>(wred_size, 1, 0) wred;
    @name(".egress_wred.set_wred_index") action set_wred_index(switch_wred_index_t wred_index) {
        index = wred_index;
        wred_flag = (bit<1>)wred.execute(local_md.qos.qdepth, wred_index);
    }
    @name(".egress_wred.wred_index") table wred_index {
        key = {
            eg_intr_md.egress_port: exact @name("port");
            local_md.qos.qid      : exact @name("qid");
            local_md.qos.color    : exact @name("color");
        }
        actions = {
            NoAction;
            set_wred_index;
        }
        const default_action = NoAction;
        size = wred_size;
    }
    @name(".egress_wred.set_ipv4_ecn") action set_ipv4_ecn() {
        hdr.ipv4.diffserv[1:0] = SWITCH_ECN_CODEPOINT_CE;
        wred_drop = false;
    }
    @name(".egress_wred.set_ipv6_ecn") action set_ipv6_ecn() {
        hdr.ipv6.traffic_class[1:0] = SWITCH_ECN_CODEPOINT_CE;
        wred_drop = false;
    }
    @name(".egress_wred.drop") action drop() {
        wred_drop = true;
    }
    @name(".egress_wred.v4_wred_action") table v4_wred_action {
        key = {
            index                 : exact;
            hdr.ipv4.diffserv[1:0]: exact;
        }
        actions = {
            NoAction;
            drop;
            set_ipv4_ecn;
        }
        size = 3 * wred_size;
    }
    @name(".egress_wred.v6_wred_action") table v6_wred_action {
        key = {
            index                      : exact;
            hdr.ipv6.traffic_class[1:0]: exact;
        }
        actions = {
            NoAction;
            drop;
            set_ipv6_ecn;
        }
        size = 3 * wred_size;
    }
    @name(".egress_wred.count") action count() {
        stats.count();
    }
    @ways(2) @name(".egress_wred.wred_stats") table wred_stats {
        key = {
            eg_intr_md.egress_port: exact @name("port");
            local_md.qos.qid      : exact @name("qid");
            local_md.qos.color    : exact @name("color");
            wred_drop             : exact;
        }
        actions = {
            @defaultonly NoAction;
            count;
        }
        const default_action = NoAction;
        size = 2 * wred_size;
        counters = stats;
    }
    apply {
        if (!local_md.flags.bypass_egress) {
            wred_index.apply();
        }
        if (!local_md.flags.bypass_egress && wred_flag == 1) {
            if (hdr.ipv4.isValid()) {
                switch (v4_wred_action.apply().action_run) {
                    NoAction: {
                    }
                    default: {
                        wred_stats.apply();
                    }
                }
            } else if (hdr.ipv6.isValid()) {
                switch (v6_wred_action.apply().action_run) {
                    NoAction: {
                    }
                    default: {
                        wred_stats.apply();
                    }
                }
            }
        }
    }
}

control IngressDestNatPool(inout switch_local_metadata_t local_md) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS) dnat_pool_stats;
    @name(".set_dnat_pool_hit") action set_dnat_pool_hit() {
        local_md.nat.dnat_pool_hit = true;
        dnat_pool_stats.count();
    }
    @name(".set_dnat_pool_miss") action set_dnat_pool_miss() {
        local_md.nat.dnat_pool_hit = false;
        dnat_pool_stats.count();
    }
    @name(".dnat_pool") table dnat_pool {
        key = {
            local_md.lkp.ip_dst_addr[95:64]: exact;
        }
        actions = {
            set_dnat_pool_hit;
            set_dnat_pool_miss;
        }
        counters = dnat_pool_stats;
        const default_action = set_dnat_pool_miss;
        size = DNAT_POOL_TABLE_SIZE;
    }
    apply {
        dnat_pool.apply();
    }
}

control IngressDnaptIndex(inout switch_local_metadata_t local_md)(switch_uint32_t dnapt_table_size) {
    @name(".set_dnapt_index") action set_dnapt_index(bit<16> index) {
        local_md.nat.dnapt_index = index;
    }
    @name(".dest_napt_index") table dest_napt {
        key = {
            local_md.lkp.ip_dst_addr[95:64]: exact;
            local_md.lkp.ip_proto          : exact;
            local_md.lkp.l4_dst_port       : exact;
        }
        actions = {
            set_dnapt_index;
            NoAction;
        }
        const default_action = NoAction;
        size = dnapt_table_size;
        idle_timeout = true;
    }
    apply {
        dest_napt.apply();
    }
}

control IngressSnaptIndex(inout switch_local_metadata_t local_md)(switch_uint32_t snapt_table_size) {
    @name(".set_snapt_index") action set_snapt_index(bit<16> index) {
        local_md.nat.snapt_index = index;
    }
    @name(".src_napt_index") @pack(2) table src_napt {
        key = {
            local_md.lkp.ip_src_addr[95:64]: exact;
            local_md.lkp.ip_proto          : exact;
            local_md.lkp.l4_src_port       : exact;
        }
        actions = {
            set_snapt_index;
            NoAction;
        }
        const default_action = NoAction;
        size = snapt_table_size;
        idle_timeout = true;
    }
    apply {
        src_napt.apply();
    }
}

control IngressNat(inout switch_local_metadata_t local_md) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) flow_napt_stats;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) flow_nat_stats;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) dest_napt_stats;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) dest_nat_stats;
    @name(".flow_napt_miss") action flow_napt_miss() {
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_FLOW_NONE;
        flow_napt_stats.count();
    }
    @name(".dnapt_miss") action dnapt_miss() {
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_DEST_NONE;
        dest_napt_stats.count();
    }
    @name(".flow_nat_miss") action flow_nat_miss() {
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_FLOW_NONE;
        flow_nat_stats.count();
    }
    @name(".dnat_miss") action dnat_miss() {
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_DEST_NONE;
        dest_nat_stats.count();
    }
    @name(".set_flow_nat_rewrite") action set_flow_nat_rewrite(ipv4_addr_t sip, ipv4_addr_t dip) {
        local_md.lkp.ip_src_addr[95:64] = sip;
        local_md.lkp.ip_dst_addr[95:64] = dip;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_FLOW_NAT;
        flow_nat_stats.count();
    }
    @name(".set_flow_napt_rewrite") action set_flow_napt_rewrite(ipv4_addr_t sip, ipv4_addr_t dip, bit<16> sport, bit<16> dport) {
        local_md.lkp.ip_src_addr[95:64] = sip;
        local_md.lkp.ip_dst_addr[95:64] = dip;
        local_md.lkp.l4_src_port = sport;
        local_md.lkp.l4_dst_port = dport;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_FLOW_NAPT;
        flow_napt_stats.count();
    }
    @name(".set_dnapt_rewrite") action set_dnapt_rewrite(ipv4_addr_t dip, bit<16> dport) {
        local_md.lkp.ip_dst_addr[95:64] = dip;
        local_md.lkp.l4_dst_port = dport;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_DEST_NAPT;
        dest_napt_stats.count();
    }
    @name(".set_dnat_rewrite") action set_dnat_rewrite(ipv4_addr_t dip) {
        local_md.lkp.ip_dst_addr[95:64] = dip;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_DEST_NAT;
        dest_nat_stats.count();
    }
    @name(".dest_napt") table dest_napt {
        key = {
            local_md.lkp.ip_dst_addr[95:64]: exact;
            local_md.lkp.ip_proto          : exact;
            local_md.lkp.l4_dst_port       : exact;
        }
        actions = {
            set_dnapt_rewrite;
            dnapt_miss;
        }
        const default_action = dnapt_miss;
        size = DNAPT_TABLE_SIZE;
        counters = dest_napt_stats;
        idle_timeout = true;
    }
    @name(".dest_nat") table dest_nat {
        key = {
            local_md.lkp.ip_dst_addr[95:64]: exact;
        }
        actions = {
            set_dnat_rewrite;
            dnat_miss;
        }
        const default_action = dnat_miss;
        size = DNAT_TABLE_SIZE;
        counters = dest_nat_stats;
        idle_timeout = true;
    }
    @name(".flow_napt") table flow_napt {
        key = {
            local_md.lkp.ip_src_addr[95:64]: exact;
            local_md.lkp.ip_dst_addr[95:64]: exact;
            local_md.lkp.ip_proto          : exact;
            local_md.lkp.l4_dst_port       : exact;
            local_md.lkp.l4_src_port       : exact;
        }
        actions = {
            set_flow_napt_rewrite;
            flow_napt_miss;
        }
        const default_action = flow_napt_miss;
        size = FLOW_NAPT_TABLE_SIZE;
        counters = flow_napt_stats;
        idle_timeout = true;
    }
    @name(".flow_nat") table flow_nat {
        key = {
            local_md.lkp.ip_src_addr[95:64]: exact;
            local_md.lkp.ip_dst_addr[95:64]: exact;
        }
        actions = {
            set_flow_nat_rewrite;
            flow_nat_miss;
        }
        const default_action = flow_nat_miss;
        size = FLOW_NAT_TABLE_SIZE;
        counters = flow_nat_stats;
        idle_timeout = true;
    }
    apply {
        if (local_md.nat.nat_disable == false) {
            if (local_md.nat.dnat_pool_hit == true && local_md.nat.ingress_zone == SWITCH_NAT_OUTSIDE_ZONE_ID) {
                switch (dest_napt.apply().action_run) {
                    set_dnapt_rewrite: {
                    }
                    dnapt_miss: {
                        dest_nat.apply();
                    }
                }
            }
        }
    }
}

control SourceNat(inout switch_local_metadata_t local_md) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) src_napt_stats;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) src_nat_stats;
    @name(".set_snapt_rewrite") action set_snapt_rewrite(ipv4_addr_t sip, bit<16> sport) {
        local_md.lkp.ip_src_addr[95:64] = sip;
        local_md.lkp.l4_src_port = sport;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_SRC_NAPT;
        src_napt_stats.count();
    }
    @name(".set_snat_rewrite") action set_snat_rewrite(ipv4_addr_t sip) {
        local_md.lkp.ip_src_addr[95:64] = sip;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_SRC_NAT;
        src_nat_stats.count();
    }
    @name(".source_napt_miss") action source_napt_miss() {
        src_napt_stats.count();
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_SRC_NONE;
    }
    @name(".source_nat_miss") action source_nat_miss() {
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_SRC_NONE;
        src_nat_stats.count();
    }
    @name(".source_nat") table source_nat {
        key = {
            local_md.lkp.ip_src_addr[95:64]: exact;
        }
        actions = {
            set_snat_rewrite;
            source_nat_miss;
        }
        const default_action = source_nat_miss;
        size = SNAT_TABLE_SIZE;
        counters = src_nat_stats;
        idle_timeout = true;
    }
    @name(".source_napt") table source_napt {
        key = {
            local_md.lkp.ip_src_addr[95:64]: exact;
            local_md.lkp.ip_proto          : exact;
            local_md.lkp.l4_src_port       : exact;
        }
        actions = {
            set_snapt_rewrite;
            source_napt_miss;
        }
        const default_action = source_napt_miss;
        size = SNAPT_TABLE_SIZE;
        counters = src_napt_stats;
        idle_timeout = true;
    }
    apply {
        if (local_md.nat.ingress_zone == SWITCH_NAT_INSIDE_ZONE_ID && local_md.nat.nat_disable == false) {
            switch (source_napt.apply().action_run) {
                set_snapt_rewrite: {
                }
                source_napt_miss: {
                    source_nat.apply();
                }
            }
        }
    }
}

control IngressNatRewrite(inout switch_header_t hdr, inout switch_local_metadata_t local_md) {
    @name(".rewrite_tcp_flow") action rewrite_tcp_flow() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
        hdr.tcp.src_port = local_md.lkp.l4_src_port;
        hdr.tcp.dst_port = local_md.lkp.l4_dst_port;
    }
    @name(".rewrite_udp_flow") action rewrite_udp_flow() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
        hdr.udp.src_port = local_md.lkp.l4_src_port;
        hdr.udp.dst_port = local_md.lkp.l4_dst_port;
    }
    @name(".rewrite_ipsa_ipda") action rewrite_ipsa_ipda() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
    }
    @name(".rewrite_tcp_ipda") action rewrite_tcp_ipda() {
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
        hdr.tcp.dst_port = local_md.lkp.l4_dst_port;
    }
    @name(".rewrite_udp_ipda") action rewrite_udp_ipda() {
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
        hdr.udp.dst_port = local_md.lkp.l4_dst_port;
    }
    @name(".rewrite_ipda") action rewrite_ipda() {
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
    }
    @name(".rewrite_ipsa") action rewrite_ipsa() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
    }
    @name(".rewrite_tcp_ipsa") action rewrite_tcp_ipsa() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
        hdr.tcp.src_port = local_md.lkp.l4_src_port;
    }
    @name(".rewrite_udp_ipsa") action rewrite_udp_ipsa() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
        hdr.udp.src_port = local_md.lkp.l4_src_port;
    }
    @name(".ingress_nat_rewrite") table ingress_nat_rewrite {
        key = {
            local_md.nat.hit               : exact;
            local_md.lkp.ip_proto          : exact;
            local_md.checks.same_zone_check: ternary;
        }
        actions = {
            rewrite_tcp_flow;
            rewrite_udp_flow;
            rewrite_ipsa_ipda;
            rewrite_tcp_ipda;
            rewrite_udp_ipda;
            rewrite_ipda;
            rewrite_tcp_ipsa;
            rewrite_udp_ipsa;
            rewrite_ipsa;
        }
        size = INGRESS_NAT_REWRITE_TABLE_SIZE;
    }
    apply {
        ingress_nat_rewrite.apply();
    }
}

struct switch_sflow_info_t {
    bit<32> current;
    bit<32> rate;
}

control IngressSflow(inout switch_local_metadata_t local_md) {
    const bit<32> sflow_session_size = 256;
    @name(".ingress_sflow_samplers") Register<switch_sflow_info_t, bit<32>>(sflow_session_size) samplers;
    RegisterAction<switch_sflow_info_t, bit<8>, bit<1>>(samplers) sample_packet = {
        void apply(inout switch_sflow_info_t reg, out bit<1> flag) {
            if (reg.current > 0) {
                reg.current = reg.current - 1;
            } else {
                reg.current = reg.rate;
                flag = 1;
            }
        }
    };
    apply {
        if (local_md.sflow.session_id != SWITCH_SFLOW_INVALID_ID) {
            local_md.sflow.sample_packet = sample_packet.execute(local_md.sflow.session_id);
        }
    }
}

control EgressSflow(inout switch_local_metadata_t local_md) {
    const bit<32> sflow_session_size = 256;
    Register<switch_sflow_info_t, bit<32>>(sflow_session_size) samplers;
    RegisterAction<switch_sflow_info_t, bit<8>, bit<1>>(samplers) sample_packet = {
        void apply(inout switch_sflow_info_t reg, out bit<1> flag) {
            if (reg.current > 0) {
                reg.current = reg.current - 1;
            } else {
                reg.current = reg.rate;
                flag = 1;
            }
        }
    };
    apply {
    }
}

@pa_container_size("ingress" , "local_md.lag_hash" , 16) @pa_container_size("ingress" , "ig_intr_md_for_tm.level2_mcast_hash" , 16) @pa_container_size("ingress" , "ig_intr_md_for_tm.rid" , 16) @pa_container_size("ingress" , "ig_intr_md_for_tm.ingress_cos" , 16) control SwitchIngress(inout switch_header_t hdr, inout switch_local_metadata_t local_md, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_intr_from_prsr, inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr, inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm) {
    IngressPortMapping(PORT_VLAN_TABLE_SIZE, BD_TABLE_SIZE) ingress_port_mapping;
    PktValidation() pkt_validation;
    SMAC(MAC_TABLE_SIZE) smac;
    DMAC(MAC_TABLE_SIZE) dmac;
    IngressTunnel() tunnel;
    IngressBd(BD_TABLE_SIZE) bd_stats;
    EnableFragHash() enable_frag_hash;
    Ipv4Hash() ipv4_hash;
    Ipv6Hash() ipv6_hash;
    OuterIpv4Hash() outer_ipv4_hash;
    OuterIpv6Hash() outer_ipv6_hash;
    NonIpHash() non_ip_hash;
    Lagv4Hash() lagv4_hash;
    Lagv6Hash() lagv6_hash;
    LOU() lou;
    Fibv4(IPV4_HOST_TABLE_SIZE, IPV4_LPM_TABLE_SIZE) ipv4_fib;
    Fibv6(IPV6_HOST_TABLE_SIZE, IPV6_HOST64_TABLE_SIZE, IPV6_LPM_TABLE_SIZE, IPV6_LPM64_TABLE_SIZE) ipv6_fib;
    IngressIpv4Acl(INGRESS_IPV4_ACL_TABLE_SIZE) ingress_ipv4_acl;
    IngressIpv6Acl(INGRESS_IPV6_ACL_TABLE_SIZE) ingress_ipv6_acl;
    IngressIpAcl(INGRESS_IP_MIRROR_ACL_TABLE_SIZE) ingress_ip_mirror_acl;
    IngressMirrorMeter() ingress_mirror_meter;
    IngressQoSMap() qos_map;
    IngressTC() traffic_class;
    PPGStats() ppg_stats;
    IngressPFCWd(512) pfc_wd;
    Nexthop(NEXTHOP_TABLE_SIZE, ECMP_GROUP_TABLE_SIZE, ECMP_SELECT_TABLE_SIZE) nexthop;
    OuterFib() outer_fib;
    LAG() lag;
    MulticastFlooding(BD_FLOOD_TABLE_SIZE) flood;
    IngressSystemAcl() system_acl;
    IngressNat() ingress_nat;
    SourceNat() source_nat;
    IngressDestNatPool() ingress_dnat_pool;
    IngressNatRewrite() ingress_nat_rewrite;
    IngressTosMirrorAcl() ingress_tos_mirror_acl;
    IngressSflow() sflow;
    SameMacCheck() same_mac_check;
    SameIfCheck() same_if_check;
    apply {
        pkt_validation.apply(hdr, local_md);
        ingress_port_mapping.apply(hdr, local_md, ig_intr_md_for_tm, ig_intr_md_for_dprsr);
        smac.apply(hdr.ethernet.src_addr, hdr.ethernet.dst_addr, local_md, ig_intr_md_for_dprsr.digest_type);
        tunnel.apply(hdr, local_md, local_md.lkp);
        lou.apply(local_md);
        if (local_md.lkp.ip_type == SWITCH_IP_TYPE_IPV4) {
            ingress_dnat_pool.apply(local_md);
            ingress_ipv4_acl.apply(local_md, local_md.acl_nexthop);
            if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_NAT != 0) && local_md.tunnel.terminate == false) {
                ingress_nat.apply(local_md);
                source_nat.apply(local_md);
            }
            if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_L3 != 0) && local_md.ipv4.unicast_enable && local_md.flags.rmac_hit) {
                ipv4_fib.apply(local_md.lkp.ip_dst_addr[95:64], local_md);
            } else {
                dmac.apply(local_md.lkp.mac_dst_addr, local_md);
            }
        } else if (local_md.lkp.ip_type == SWITCH_IP_TYPE_IPV6) {
            ingress_ipv6_acl.apply(local_md, local_md.acl_nexthop);
            if (!(local_md.bypass & SWITCH_INGRESS_BYPASS_L3 != 0) && local_md.ipv6.unicast_enable && local_md.flags.rmac_hit) {
                ipv6_fib.apply(local_md.lkp.ip_dst_addr, local_md);
            } else {
                dmac.apply(local_md.lkp.mac_dst_addr, local_md);
            }
        } else {
            if (local_md.lkp.ip_type != SWITCH_IP_TYPE_IPV6) {
                ingress_ipv4_acl.apply(local_md, local_md.acl_nexthop);
            }
            if (local_md.lkp.ip_type != SWITCH_IP_TYPE_IPV4) {
                ingress_ipv6_acl.apply(local_md, local_md.acl_nexthop);
            }
            dmac.apply(local_md.lkp.mac_dst_addr, local_md);
        }
        ingress_ip_mirror_acl.apply(local_md, local_md.unused_nexthop);
        if (local_md.mirror.session_id == 0) {
            ingress_tos_mirror_acl.apply(local_md);
        }
        same_mac_check.apply(hdr, local_md);
        ingress_mirror_meter.apply(local_md);
        sflow.apply(local_md);
        bd_stats.apply(local_md.bd, local_md.lkp.pkt_type);
        enable_frag_hash.apply(local_md.lkp);
        if (local_md.lkp.ip_type == SWITCH_IP_TYPE_NONE) {
            non_ip_hash.apply(hdr, local_md, local_md.lag_hash);
        } else if (local_md.lkp.ip_type == SWITCH_IP_TYPE_IPV4) {
            lagv4_hash.apply(local_md.lkp, local_md.lag_hash);
        } else {
            lagv6_hash.apply(local_md.lkp, local_md.lag_hash);
        }
        if (local_md.lkp.ip_type == SWITCH_IP_TYPE_IPV4) {
            ipv4_hash.apply(local_md.lkp, local_md.ecmp_hash);
            outer_ipv4_hash.apply(local_md.lkp, local_md.outer_ecmp_hash);
        } else {
            ipv6_hash.apply(local_md.lkp, local_md.ecmp_hash);
            outer_ipv6_hash.apply(local_md.lkp, local_md.outer_ecmp_hash);
        }
        nexthop.apply(local_md);
        same_if_check.apply(local_md);
        qos_map.apply(hdr, local_md);
        traffic_class.apply(local_md);
        ppg_stats.apply(local_md);
        outer_fib.apply(local_md);
        if (local_md.egress_port_lag_index == SWITCH_FLOOD) {
            flood.apply(local_md);
        } else {
            lag.apply(local_md, local_md.lag_hash, ig_intr_md_for_tm.ucast_egress_port);
        }
        pfc_wd.apply(local_md.ingress_port, local_md.qos.qid, local_md.flags.pfc_wd_drop);
        system_acl.apply(hdr, local_md, ig_intr_md_for_tm, ig_intr_md_for_dprsr);
        add_bridged_md(hdr.bridged_md, local_md);
        if (local_md.mirror.src == SWITCH_PKT_SRC_CLONED_EGRESS) {
            copy_mirror_to_bridged_md(hdr.bridged_md, local_md);
        }
        if (local_md.nat.hit != SWITCH_NAT_HIT_NONE) {
            ingress_nat_rewrite.apply(hdr, local_md);
        }
        set_ig_intr_md(local_md, ig_intr_md_for_dprsr, ig_intr_md_for_tm);
    }
}

control SwitchEgress(inout switch_header_t hdr, inout switch_local_metadata_t local_md, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr, inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
    EgressPortMapping() egress_port_mapping;
    EgressPortMirror(288) port_mirror;
    EgressLOU() lou;
    EgressQoS() qos;
    EgressQueue(EGRESS_QUEUE_TABLE_SIZE) queue;
    EgressIpv4Acl(EGRESS_IPV4_ACL_TABLE_SIZE) egress_ipv4_acl;
    EgressIpv6Acl(EGRESS_IPV6_ACL_TABLE_SIZE) egress_ipv6_acl;
    EgressIpv4Acl(EGRESS_IPV4_MIRROR_ACL_TABLE_SIZE) egress_ipv4_mirror_acl;
    EgressIpv6Acl(EGRESS_IPV6_MIRROR_ACL_TABLE_SIZE) egress_ipv6_mirror_acl;
    EgressMirrorMeter() egress_mirror_meter;
    EgressSystemAcl() system_acl;
    EgressPFCWd(512) pfc_wd;
    EgressVRF() egress_vrf;
    EgressBD() egress_bd;
    OuterNexthop() outer_nexthop;
    EgressBDStats() egress_bd_stats;
    MirrorRewrite() mirror_rewrite;
    VlanXlate(VLAN_TABLE_SIZE, PORT_VLAN_TABLE_SIZE) vlan_xlate;
    VlanDecap() vlan_decap;
    TunnelDecap() tunnel_decap;
    TunnelNexthop() tunnel_nexthop;
    TunnelEncap() tunnel_encap;
    TunnelRewrite() tunnel_rewrite;
    MTU() mtu;
    WRED() wred;
    EgressCpuRewrite() cpu_rewrite;
    EgressPortIsolation() port_isolation;
    Neighbor() neighbor;
    VniMap() vni_map;
    SetEgIntrMd() set_eg_intr_md;
    EgressTosMirrorAcl() egress_tos_mirror_acl;
    apply {
        egress_port_mapping.apply(hdr, local_md, eg_intr_md_for_dprsr, eg_intr_md.egress_port);
        if (local_md.pkt_src == SWITCH_PKT_SRC_BRIDGED) {
            port_mirror.apply(eg_intr_md.egress_port, local_md);
            if (local_md.tunnel.terminate) {
                tunnel_decap.apply(hdr, local_md);
            } else {
                vlan_decap.apply(hdr, local_md);
            }
            egress_vrf.apply(hdr, local_md);
            tunnel_nexthop.apply(hdr, local_md);
            vni_map.apply(hdr, local_md);
            lou.apply(local_md);
            qos.apply(hdr, eg_intr_md.egress_port, local_md);
            wred.apply(hdr, local_md, eg_intr_md, local_md.flags.wred_drop);
            outer_nexthop.apply(hdr, local_md);
            if (hdr.ipv4.isValid()) {
                egress_ipv4_acl.apply(hdr, local_md);
                egress_ipv4_mirror_acl.apply(hdr, local_md);
            } else if (hdr.ipv6.isValid()) {
                egress_ipv6_acl.apply(hdr, local_md);
                egress_ipv6_mirror_acl.apply(hdr, local_md);
            }
            tunnel_encap.apply(hdr, local_md);
            egress_bd.apply(hdr, local_md);
            if (local_md.mirror.session_id == 0) {
                egress_tos_mirror_acl.apply(hdr, local_md);
            }
            egress_mirror_meter.apply(local_md);
            tunnel_rewrite.apply(hdr, local_md);
            neighbor.apply(hdr, local_md);
        } else {
            mirror_rewrite.apply(hdr, local_md, eg_intr_md_for_dprsr);
        }
        egress_bd_stats.apply(hdr, local_md);
        mtu.apply(hdr, local_md);
        vlan_xlate.apply(hdr, local_md);
        pfc_wd.apply(eg_intr_md.egress_port, local_md.qos.qid, local_md.flags.pfc_wd_drop);
        port_isolation.apply(local_md, eg_intr_md);
        system_acl.apply(hdr, local_md, eg_intr_md, eg_intr_md_for_dprsr);
        cpu_rewrite.apply(hdr, local_md, eg_intr_md_for_dprsr, eg_intr_md.egress_port);
        set_eg_intr_md.apply(hdr, local_md, eg_intr_md_for_dprsr, eg_intr_md_for_oport);
        queue.apply(eg_intr_md.egress_port, local_md);
    }
}

Pipeline<switch_header_t, switch_local_metadata_t, switch_header_t, switch_local_metadata_t>(SwitchIngressParser(), SwitchIngress(), SwitchIngressDeparser(), SwitchEgressParser(), SwitchEgress(), SwitchEgressDeparser()) pipe;
Switch(pipe) main;
