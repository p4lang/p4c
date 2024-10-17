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

#ifndef _TOFINO2_NATIVE_ARCHITECTURE_
#define _TOFINO2_NATIVE_ARCHITECTURE_

#if   __TARGET_TOFINO__ == 2
#include "tofino2_specs.p4"
#else
#error Target does not support tofino2 native architecture
#endif

#include "tofino2_arch.p4"

#endif /* _TOFINO2_NATIVE_ARCHITECTURE_ */
