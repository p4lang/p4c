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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_SOLITARY_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_SOLITARY_H_

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

using namespace P4;

class PragmaSolitary : public Inspector {
    PhvInfo &phv_i;

    bool preorder(const IR::BFN::Pipe *pipe) override;

 public:
    explicit PragmaSolitary(PhvInfo &phv) : phv_i(phv) {}

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_SOLITARY_H_ */
