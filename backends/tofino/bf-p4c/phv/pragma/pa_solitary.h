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

#ifndef EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_SOLITARY_H_
#define EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_SOLITARY_H_

#include "ir/ir.h"
#include "bf-p4c/phv/phv_fields.h"

using namespace P4;

class PragmaSolitary : public Inspector {
    PhvInfo& phv_i;

    bool preorder(const IR::BFN::Pipe* pipe) override;
 public:
    explicit PragmaSolitary(PhvInfo& phv) : phv_i(phv) { }

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;
};

#endif /* EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_SOLITARY_H_ */
