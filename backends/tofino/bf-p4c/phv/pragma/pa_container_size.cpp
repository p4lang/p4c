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

#include "bf-p4c/phv/pragma/pa_container_size.h"

#include <numeric>
#include <string>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/log.h"

namespace {

std::optional<PHV::Size> convert_to_phv_size(const IR::Constant *ir) {
    int rst = ir->asInt();
    if (rst != 8 && rst != 16 && rst != 32) {
        warning(
            "@pragma pa_container_size's third argument "
            "must be one of {8, 16, 32}, but  %1% is not, skipped",
            rst);
        return std::nullopt;
    }
    return std::make_optional(static_cast<PHV::Size>(rst));
}

ordered_map<const PHV::Field *, std::vector<int>> compute_field_layouts(
    const ordered_map<const PHV::Field *, std::vector<PHV::Size>> &pa_container_sizes) {
    ordered_map<const PHV::Field *, std::vector<int>> rst;
    for (const auto &kv : pa_container_sizes) {
        const auto *field = kv.first;
        const auto &sizes = kv.second;
        rst[field] = {};
        if (sizes.size() == 1) {
            int sz_int = static_cast<int>(sizes.front());
            int rest_width = field->size;
            while (rest_width > 0) {
                rst[field].push_back(sz_int);
                rest_width -= sz_int;
            }
        } else {
            int sum = 0;
            for (const auto &sz : sizes) {
                sum += static_cast<int>(sz);
                rst[field].push_back(static_cast<int>(sz));
            }
            if (sum < field->size) {
                warning(
                    "@pragma pa_container_size %1% has "
                    "sum of sizes that is less than the field size",
                    field->name);
            }
        }
    }
    return rst;
}

}  // namespace

/// BFN::Pragma interface
const char *PragmaContainerSize::name = "pa_container_size";
const char *PragmaContainerSize::description =
    "Forces the allocation of a field in the specified container size.";
const char *PragmaContainerSize::help =
    "@pragma pa_container_size [pipe] gress instance_name.field_name 32\n"
    "+ attached to P4 header instances\n"
    "\n"
    "Specifies that the indicated packet or metadata field should be "
    "allocated to containers of the indicated size.\n"
    "\n"
    "If one container size is specified, and the field is larger than that "
    "container size, the field will only be allocated in containers of that "
    "size.  The field will occupy multiple containers of the same size.\n"
    "\n"
    "If multiple container sizes are specified, then the first container you "
    "specify will hold the least significant bits, and the last container "
    "you specify will hold the most significant bits. containers are filled"
    " from the least significant bit to the most significant bit. "
    "The container that holds the most significant bit may be partially filled.\n"
    "\n"
    "For example, if you have a 60b field with:\n"
    "\n"
    "@pragma pa_container_size ingress hdr1.x_60 8 16 8 28\n"
    "That means the allocation of container will be: \n"
    "\n"
    "hdr1.x_60[7:0]   --> 8 bits\n"
    "hdr1.x_60[23:8]  --> 16 bits\n"
    "hdr1.x_60[31:24] --> 8 bits\n"
    "hdr1.x_60[59:32] --> 28 bits\n"
    "\n"
    "Allowed container sizes are 8, 16, and 32.  The gress value can be "
    "either ingress or egress. "
    "If the optional pipe value is provided, the pragma is applied only "
    "to the corresponding pipeline. If not provided, it is applied to "
    "all pipelines.";

bool PragmaContainerSize::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name != PragmaContainerSize::name) continue;

        auto &exprs = annotation->expr;

        const unsigned min_required_arguments = 3;  // gress, field, size1, ...
        unsigned required_arguments = min_required_arguments;
        unsigned expr_index = 0;
        const IR::StringLiteral *pipe_arg = nullptr;
        const IR::StringLiteral *gress_arg = nullptr;

        if (!PHV::Pragmas::determinePipeGressArgs(exprs, expr_index, required_arguments, pipe_arg,
                                                  gress_arg)) {
            continue;
        }

        if (!PHV::Pragmas::checkNumberArgs(annotation, required_arguments, min_required_arguments,
                                           false, cstring(PragmaContainerSize::name),
                                           "`gress', `field', `size1'"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        auto field_ir = exprs[expr_index]->to<IR::StringLiteral>();
        if (!field_ir) {
            warning(ErrorType::WARN_INVALID,
                    "%1%: Found a non-string literal argument `field'. Ignoring pragma.",
                    exprs[expr_index]);
            continue;
        }
        expr_index++;

        // check field name
        auto field_name = gress_arg->value + "::" + field_ir->value;
        auto field = phv_i.field(field_name);
        if (!field) {
            PHV::Pragmas::reportNoMatchingPHV(pipe, field_ir, field_name);
            continue;
        }

        if (pa_container_sizes_i.count(field)) {
            warning("overriding @pragma pa_container_size %1%", field->name);
        }
        pa_container_sizes_i[field] = {};

        bool failed = false;
        for (; expr_index < exprs.size(); ++expr_index) {
            auto container_size_ir = exprs[expr_index]->to<IR::Constant>();
            if (!container_size_ir) {
                warning(ErrorType::WARN_INVALID,
                        "%1%: Found a non-integer literal argument `size'. Ignoring pragma.",
                        exprs[expr_index]);
                failed = true;
                break;
            }

            auto container_size = convert_to_phv_size(container_size_ir);
            if (!container_size) {
                failed = true;
                break;
            } else {
                pa_container_sizes_i[field].push_back(*container_size);
            }
        }

        if (failed) {
            pa_container_sizes_i.erase(field);
            continue;
        }

        check_and_add_no_split(field);
    }

    // alt-phv-alloc ONLY
    // this adds a set of pa_container_size as indicated by table summary after alt-phv-alloc's
    // table replay.
    for (const auto &it : container_size_constr) {
        auto field = phv_i.field(it.first);
        BUG_CHECK(field, "field not found");
        pa_container_sizes_i[field] = it.second;
        check_and_add_no_split(field);
    }

    LOG1(*this);
    return true;
}

void PragmaContainerSize::end_apply() {
    field_layouts_i = compute_field_layouts(pa_container_sizes_i);
}

std::optional<PHV::Size> PragmaContainerSize::expected_container_size(
    const PHV::FieldSlice &fs) const {
    if (!field_layouts_i.count(fs.field())) {
        return std::nullopt;
    }
    int offset = 0;
    for (const auto &v : field_layouts_i.at(fs.field())) {
        le_bitrange range = StartLen(offset, v);
        if (range.contains(fs.range())) {
            return PHV::Size(v);
        }
        offset += v;
    }
    return PHV::Size::null;
}

void PragmaContainerSize::check_and_add_no_split(PHV::Field *field) const {
    // Add no-split constraint if the field has only one container size associated with it and
    // the size of the container is larger than or equal to the size of the field. The pragma
    // also implies that the entire field must be packed into a single specified size container.
    if (pa_container_sizes_i.at(field).size() == 1) {
        auto container_size = pa_container_sizes_i.at(field).front();
        if (static_cast<int>(container_size) >= field->size) {
            field->set_no_split(true);
            LOG3("Setting field " << field->name << " to no-split.");
        }
        // TODO: not necessary.
        if (static_cast<int>(container_size) == field->size) {
            field->set_solitary(PHV::SolitaryReason::PRAGMA_CONTAINER_SIZE);
            LOG3("Setting field " << field->name << " to no-pack.");
        }
    }
}

std::ostream &operator<<(std::ostream &out, const PragmaContainerSize &pa_cs) {
    std::stringstream logs;
    for (const auto &kv : pa_cs.field_to_sizes()) {
        auto &field = kv.first;
        auto &container_sizes = kv.second;
        logs << "Add @pa_container_size that: " << field << " must be allocated to " << " [ ";
        for (const auto &sz : container_sizes) {
            logs << sz << " ";
        }
        logs << " ] " << " container(s)" << std::endl;
    }

    out << logs.str();
    return out;
}

void PragmaContainerSize::add_constraint(const PHV::Field *field, std::vector<PHV::Size> sizes) {
    pa_container_sizes_i[field] = sizes;
    field_layouts_i = compute_field_layouts(pa_container_sizes_i);
    PHV::Field *nonConstField = phv_i.field(field->name);
    check_and_add_no_split(nonConstField);
}

cstring PragmaContainerSize::printSizeConstraints(const std::vector<PHV::Size> &sizes) const {
    std::stringstream ss;
    ss << "[ ";
    for (auto sz : sizes) ss << sz << " ";
    ss << "]";
    return ss.str();
}

// Only used by PardePhvConstraints.
bool PragmaContainerSize::check_and_add_constraint(const PHV::Field *field,
                                                   std::vector<PHV::Size> sizes) {
    if (!pa_container_sizes_i.count(field)) {
        add_constraint(field, sizes);
        return true;
    }
    // Check if existing constraints are the same as the ones already on this field.
    // ss contains a pretty print of already existing pragmas.
    std::vector<PHV::Size> existingSizePragmas = pa_container_sizes_i.at(field);

    bool sameAsExisting = true;

    if (existingSizePragmas.size() != sizes.size()) {
        sameAsExisting = false;
    } else {
        for (size_t i = 0; i < existingSizePragmas.size(); i++) {
            if (sizes[i] != existingSizePragmas[i]) {
                sameAsExisting = false;
                break;
            }
        }
    }

    if (!sameAsExisting) {
        warning("@pragma pa_container_size already specified for field %1% %2%.", field->name,
                printSizeConstraints(existingSizePragmas));
        return false;
    }

    return true;
}
