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

#ifndef BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_P4RUNTIME_FORCE_STD_H_
#define BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_P4RUNTIME_FORCE_STD_H_

#include "control-plane/p4RuntimeSerializer.h"

namespace BFN {

using namespace P4;

/// Takes a P4Info message generated for a TNA-specific architecture and
/// converts it to a "standard" P4Info message, which can be used with a
/// standard P4Runtime implementation (with no support for TNA-specific
/// extensions). Displays a warning to the user for each extern instance that
/// cannot be converted.
P4::P4RuntimeAPI convertToStdP4Runtime(const P4::P4RuntimeAPI &p4RuntimeInput);

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_P4RUNTIME_FORCE_STD_H_ */
