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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_REWRITE_EMIT_CLOT_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_REWRITE_EMIT_CLOT_H_

#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parde_visitor.h"

namespace Parde::Lowered {

/**
 * \ingroup LowerDeparserIR
 *
 * \brief Replace Emits covered in a CLOT with EmitClot
 *
 * This pass visits the deparsers, and for each deparser, it walks through the emits and identifies
 * checksums/fields that are covered as part of a CLOT. Elements that are covered by a CLOT are
 * removed from the emit list and are replace by EmitClot objects.
 */
struct RewriteEmitClot : public DeparserModifier {
    RewriteEmitClot(const PhvInfo &phv, ClotInfo &clotInfo) : phv(phv), clotInfo(clotInfo) {}

 private:
    bool preorder(IR::BFN::Deparser *deparser) override;

    const PhvInfo &phv;
    ClotInfo &clotInfo;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_REWRITE_EMIT_CLOT_H_ */
