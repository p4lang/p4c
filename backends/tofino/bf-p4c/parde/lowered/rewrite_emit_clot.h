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

#ifndef EXTENSIONS_BF_P4C_PARDE_LOWERED_REWRITE_EMIT_CLOT_H_
#define EXTENSIONS_BF_P4C_PARDE_LOWERED_REWRITE_EMIT_CLOT_H_

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
    RewriteEmitClot(const PhvInfo& phv, ClotInfo& clotInfo) : phv(phv), clotInfo(clotInfo) {}

 private:
    bool preorder(IR::BFN::Deparser* deparser) override;

    const PhvInfo& phv;
    ClotInfo& clotInfo;
};

}  // namespace Parde::Lowered

#endif /* EXTENSIONS_BF_P4C_PARDE_LOWERED_REWRITE_EMIT_CLOT_H_ */
