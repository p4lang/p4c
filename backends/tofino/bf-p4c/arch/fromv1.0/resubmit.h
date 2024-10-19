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

#ifndef BF_P4C_ARCH_FROMV1_0_RESUBMIT_H_
#define BF_P4C_ARCH_FROMV1_0_RESUBMIT_H_

#include <utility>
#include <vector>

#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/cstring.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace BFN {

struct FieldPacking;

using ResubmitSources = IR::Vector<IR::Expression>;
using ResubmitExtracts = std::map<unsigned, std::pair<cstring, const ResubmitSources *>>;

class FixupResubmitMetadata : public PassManager {
    ResubmitExtracts fieldExtracts;

 public:
    FixupResubmitMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap);
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_FROMV1_0_RESUBMIT_H_ */
