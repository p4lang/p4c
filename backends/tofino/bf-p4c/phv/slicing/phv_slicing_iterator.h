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

#ifndef BF_P4C_PHV_SLICING_PHV_SLICING_ITERATOR_H_
#define BF_P4C_PHV_SLICING_PHV_SLICING_ITERATOR_H_

#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/action_packing_validator_interface.h"
#include "bf-p4c/phv/parser_packing_validator_interface.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/slicing/types.h"
#include "bf-p4c/phv/utils/utils.h"

namespace PHV {
namespace Slicing {

/// ItrContext holds the current context for the generated slicing iterator.
/// Note that, the SlicingIterator here is making slicing decision on *SliceList*
/// intead of SuperClusters. SuperClusters are just products of doing union-find on
/// sliced SliceLists with rotational clusters. see README.md for more details.
/// The input @p sc is better to be:
/// (1) split by pa_solitary already.
/// (2) split by deparsed_bottom_bits already.
class ItrContext : public IteratorInterface {
 private:
    IteratorInterface *pImpl;

 public:
    ItrContext(const PhvInfo &phv, const MapFieldToParserStates &fs, const CollectParserInfo &pi,
               const SuperCluster *sc, const PHVContainerSizeLayout &pa,
               const ActionPackingValidatorInterface &action_packing_validator,
               const ParserPackingValidatorInterface &parser_packing_validator,
               const PackConflictChecker pack_conflict, const IsReferencedChecker is_referenced);

    /// iterate will pass valid slicing results to cb. Stop when cb returns false.
    void iterate(const IterateCb &cb) override { pImpl->iterate(cb); }

    /// invalidate is the feedback mechanism for allocation algorithm to
    /// ask iterator to not produce slicing result contains @p sl. This function can be called
    /// multiple times, and the implementation decides which one will be respected.
    /// For example, a DFS slicing iterator may choose to respect the list of top-most stack frame,
    /// i.e., the most recent decision made by DFS.
    void invalidate(const SuperCluster::SliceList *sl) override { pImpl->invalidate(sl); }

    /// set iterator configs.
    void set_config(const IteratorConfig &cfg) override { pImpl->set_config(cfg); };
};

}  // namespace Slicing
}  // namespace PHV

#endif /* BF_P4C_PHV_SLICING_PHV_SLICING_ITERATOR_H_ */
