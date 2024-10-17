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

#ifndef EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_CONTAINER_SIZE_H_
#define EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_CONTAINER_SIZE_H_

#include <map>
#include <optional>
#include "ir/ir.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"

using namespace P4;

/** pa_container_size pragma support.
 *
 * This pass will gather all pa_container_size prama and generate:
 * 1. pa_container_sizes_i: map specified fields to specified sizes;
 * 2. field_layouts_i: expected container layout of fields.
 *
 * Use field_slice_req() when allocating a fieldslice to a container.
 */
class PragmaContainerSize : public Inspector {
    // This pass may mutate field properties, e.g., add no_split on field.
    PhvInfo& phv_i;
    // alt-phv-alloc ONLY
    // this is a set of pa_container_size to fix problematic tables during alt-phv-alloc's table
    // replay stage.
    const ordered_map<cstring, std::vector<PHV::Size>> &container_size_constr;
    // pa_container_sizes_i pragmas collected from program.
    ordered_map<const PHV::Field*, std::vector<PHV::Size>> pa_container_sizes_i;
    // field_layouts_i is the expected container layout of the field. It is different
    // from the above raw pa_container_sizes, e.g. @pa_container_size("f1<32>", 8) will make
    // pa_container_sizes_i = {f1: [B]}
    // field_layouts_i = {f1: [8, 8, 8, 8]}.
    ordered_map<const PHV::Field*, std::vector<int>> field_layouts_i;

    profile_t init_apply(const IR::Node* root) override {
        profile_t rv = Inspector::init_apply(root);
        pa_container_sizes_i.clear();
        field_layouts_i.clear();
        return rv;
    }

    /// Get global pragma pa_container_size.
    bool preorder(const IR::BFN::Pipe* pipe) override;

    /// Populate field_layouts_i based on pa_container_sizes_i.
    void end_apply() override;

    // Add no-split constraint if the field has only one container size associated with it adn the
    // size of the container is larger than or equal to the size of the field. The pragma also
    // implies that the entire field must be packed into a single specified size container.
    void check_and_add_no_split(PHV::Field* field) const;

 public:
    PragmaContainerSize(PhvInfo& phv,
      const ordered_map<cstring, std::vector<PHV::Size>> &container_size_constr) :
      phv_i(phv), container_size_constr(container_size_constr) { }
    explicit PragmaContainerSize(PhvInfo& phv) :
      phv_i(phv),container_size_constr(*(new ordered_map<cstring, std::vector<PHV::Size>>)){ }

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;

    const ordered_map<const PHV::Field*, std::vector<PHV::Size>>&
    field_to_sizes() const { return pa_container_sizes_i; }

    // field_to_layout return the container layout of the field.
    // e.g.
    // "f1" => [8,8,8,8], @pa_container_size("f1<32>", 8)
    // "f2" => [8,16], @pa_container_size("f2<16>", 8, 16).
    const ordered_map<const PHV::Field*, std::vector<int>>&
    field_to_layout() const { return field_layouts_i; }

    // expected_container_size returns the expected container size for this fs.
    // if not specified, return std::nullopt, and
    // if @p fs spans over more than one container, return PHV::Size::null
    std::optional<PHV::Size> expected_container_size(const PHV::FieldSlice& fs) const;

    bool is_specified(const PHV::Field* field) const { return pa_container_sizes_i.count(field); }

    /**
     *  Add constraint regardless of whether constraint is already specified on the field.
     */
    void add_constraint(const PHV::Field* field, std::vector<PHV::Size> sizes);

    /** Pretty print existing size requirements specified in vector @p sizes.
      */
    cstring printSizeConstraints(const std::vector<PHV::Size>& sizes) const;

    /** Check if the @p sizes constraints to be added to @p field violates already existing
     * constraints for that field. If it does, return false. Otherwise, add the constraint
     * and return true. Otherwise, add the constraint.
     */
    bool check_and_add_constraint(const PHV::Field* field, std::vector<PHV::Size> sizes);
};

std::ostream& operator<<(std::ostream& out, const PragmaContainerSize& pa_cs);

#endif /* EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_CONTAINER_SIZE_H_ */
