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

#ifndef EXTENSIONS_BF_P4C_ARCH_FROMV1_0_WRED_H_
#define EXTENSIONS_BF_P4C_ARCH_FROMV1_0_WRED_H_

#include "frontends/p4-14/fromv1.0/converters.h"

namespace P4 {
namespace P4V1 {

class WREDConverter : public ExternConverter {
    WREDConverter();
    static WREDConverter singleton;
 public:
    const IR::Type_Extern *convertExternType(P4V1::ProgramStructure *,
                const IR::Type_Extern *, cstring) override;
    const IR::Declaration_Instance *convertExternInstance(P4V1::ProgramStructure *,
                const IR::Declaration_Instance *, cstring,
                IR::IndexedVector<IR::Declaration> *) override;
    const IR::Statement *convertExternCall(P4V1::ProgramStructure *,
                const IR::Declaration_Instance *, const IR::Primitive *) override;
};

}  // namespace P4V1
}  // namespace P4

#endif /* EXTENSIONS_BF_P4C_ARCH_FROMV1_0_WRED_H_ */
