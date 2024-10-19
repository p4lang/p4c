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

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_REGISTER_ACTIONS_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_REGISTER_ACTIONS_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace BFN {

/**
 * \ingroup midend
 * \brief PassManager that checks if the RegisterActions work on proper Registers.
 *
 * Registers and their RegisterActions use don't care types that are unified. This
 * pass just checks if the RegisterAction works with register that has the same width.
 * If not we can still allow it, but only if the index width is appropriately increased/decreased.
 */
class CheckRegisterActions : public Inspector {
    P4::TypeMap *typeMap;

    bool preorder(const IR::Declaration_Instance *di) override;

 public:
    explicit CheckRegisterActions(P4::TypeMap *typeMap) : typeMap(typeMap) {}
};

}  // namespace BFN

#endif  // BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_REGISTER_ACTIONS_H_
