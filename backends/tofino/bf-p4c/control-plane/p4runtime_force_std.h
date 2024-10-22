/**
 * Copyright (C) 2024 Intel Corporation
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 * 
 * 
 * SPDX-License-Identifier: Apache-2.0
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
