// SPDX-FileCopyrightText: 2026 Abhishek Agarwal
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MIDEND_ELIMDEADSETINVALID_H_
#define MIDEND_ELIMDEADSETINVALID_H_

#include "ir/pass_manager.h"

namespace P4 {

/// Removes setInvalid() calls on local headers whose valid bit is never read afterwards.
/// Only locals are handled, as their validity is not visible once the parser or control
/// ends, including after a failed extract. Reading a field does not count as reading the
/// valid bit, since a field of an invalid header has an unspecified value.
/// @pre Runs after inlining and RemoveReturns, on IR whose expressions carry types.
class ElimDeadSetInvalid : public PassManager {
 public:
    ElimDeadSetInvalid();
};

}  // namespace P4

#endif /* MIDEND_ELIMDEADSETINVALID_H_ */
