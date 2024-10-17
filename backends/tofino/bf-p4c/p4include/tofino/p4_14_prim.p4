/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#if defined(_TRANSLATE_TO_V1MODEL) || defined(_V1_MODEL_P4_)
/* v1model uses HashAlgorithm while TNA uses HashAlgorithm_t, so we use this
 * hack to make them look the same */
#define HashAlgorithm_t HashAlgorithm
#endif /* _TRANSLATE_TO_V1MODEL */

/* primitives supported by tofino and not supported by v1model.p4 */
#if !defined(_V1_MODEL_P4_) && !defined(_STRATUM_P4_)
extern CloneType;  // "forward" declaration -- so parser recognizes it as a type name
extern HashAlgorithm_t;
#endif /* !_V1_MODEL_P4_ */

extern void bypass_egress();

extern void recirculate_raw(in bit<9> port);

/* used to translate p4-14 invalidate() in frontend */
#if defined(_TRANSLATE_TO_V1MODEL)
extern void invalidate<T>(in T field);
#endif
extern void invalidate_raw(in bit<9> field);
extern void invalidate_digest();

extern void sample3(in CloneType type, in bit<32> session, in bit<32> length);
extern void sample4<T>(in CloneType type, in bit<32> session, in bit<32> length, in T data);

extern void execute_meter_with_color<M, I, T>(M m, I index, out T dst, in T pre_color);

@pure extern HashAlgorithm_t random_hash(bool msb, bool extend);
@pure extern HashAlgorithm_t identity_hash(bool msb, bool extend);
// extern HashAlgorithm_t crc_poly<T>(T coeff, bool rev, bool msb, @optional T init, @optional T xor);
@pure extern HashAlgorithm_t crc_poly<T>(bool msb, bool extend, T coeff, T init, T xor, bool rev);
