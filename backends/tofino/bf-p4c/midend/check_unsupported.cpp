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

#include "bf-p4c/midend/check_unsupported.h"

#include "bf-p4c/device.h"
#include "bf-p4c/lib/safe_width.h"

namespace BFN {

bool CheckUnsupported::preorder(const IR::PathExpression *path_expression) {
    static const IR::Path SAMPLE3(IR::ID("sample3"), false);
    static const IR::Path SAMPLE4(IR::ID("sample4"), false);

    if (*path_expression->path == SAMPLE3 || *path_expression->path == SAMPLE4) {
        error(ErrorType::ERR_UNSUPPORTED, "Primitive %1% is not supported by the backend",
              path_expression->path);
        return false;
    }
    return true;
}

bool CheckUnsupported::preorder(const IR::Declaration_Instance *instance) {
    if (instance->annotations->getSingle("symmetric"_cs) != nullptr) {
        cstring type_name;
        if (auto type = instance->type->to<IR::Type_Specialized>()) {
            type_name = type->baseType->path->name;
        } else if (auto type = instance->type->to<IR::Type_Name>()) {
            type_name = type->path->name;
        } else {
            error("%s: Unexpected type for instance %s", instance->srcInfo, instance->name);
        }
        if (type_name != "Hash") {
            error(ErrorType::ERR_UNSUPPORTED, "%s: @symmetric is only supported by the Hash extern",
                  instance->srcInfo);
            return false;
        }
    }
    return true;
}

bool hasAtcamPragma(const IR::P4Table *const table_ptr) {
    for (const auto *annotation : table_ptr->annotations->annotations) {
        if (annotation->name.name.startsWith("atcam") ||
            annotation->name.originalName.startsWith("atcam"))
            return true;
    }
    return false;
}

void CheckUnsupported::postorder(const IR::P4Table *const table_ptr) {
    if (const auto *const key_ptr = table_ptr->getKey()) {
        int lpm_key_count = 0, ternary_key_count = 0, range_key_count = 0;
        size_t total_TCAM_key_bits = 0u;

        for (const auto *const key_element_ptr : key_ptr->keyElements) {
            if ("lpm" == key_element_ptr->matchType->path->name) {
                ++lpm_key_count;
            }
            if ("range" == key_element_ptr->matchType->path->name) {
                ++range_key_count;
            }
            if ("ternary" == key_element_ptr->matchType->path->name) {
                ++ternary_key_count;
                const size_t size = table_ptr->getSizeProperty()->asUint64(),
                             this_TCAM_key_bits =
                                 safe_width_bits(key_element_ptr->expression->type);

                total_TCAM_key_bits += this_TCAM_key_bits;

                LOG9("found ternary key " << '"' << key_element_ptr->expression->toString() << '"'
                                          << " of width " << this_TCAM_key_bits
                                          << " bits and table size = " << size);
            }
        }

        if (lpm_key_count > 1) {
            error(ErrorType::ERR_UNSUPPORTED,
                  "%1%table %2% Cannot match on more than one LPM"
                  " field.",
                  table_ptr->srcInfo, table_ptr->name.originalName);
        }

        if (lpm_key_count != 0 && ternary_key_count != 0) {
            error(ErrorType::ERR_UNSUPPORTED,
                  "%1%table %2% Cannot match on both ternary and LPM"
                  " fields.",
                  table_ptr->srcInfo, table_ptr->name.originalName);
        }

        if (lpm_key_count != 0 && range_key_count != 0) {
            error(ErrorType::ERR_UNSUPPORTED,
                  "%1%table %2% Cannot match on both range and LPM"
                  " fields.",
                  table_ptr->srcInfo, table_ptr->name.originalName);
        }

        if (ternary_key_count > 0) {
            // P4_14 expresses ATCAM tables as TCAM tables with an ATCAM-specific pragma,
            //   so we need to be extra-careful not to error out erroneously on those,
            //   at least for so long as we are still supporting P4_14 inputs to this compiler.
            const bool has_ATCAM_pragma = hasAtcamPragma(table_ptr);
            if (has_ATCAM_pragma) return;  //  no further checks

            const size_t table_size = table_ptr->getSizeProperty()->asUint64();
            LOG5("found " << ternary_key_count
                          << " ternary key(s) in a table with {total TCAM key"
                             " bits = "
                          << total_TCAM_key_bits << "} and {table size = " << table_size << "}");

            const auto &MAU_spec = Device::mauSpec();

            const size_t inclusive_max_size_in_bits =
                MAU_spec.tcam_rows() * MAU_spec.tcam_columns() * MAU_spec.tcam_width() *
                MAU_spec.tcam_depth() * Device::numStages();

            LOG9("TCAM inclusive_max_size_in_bits = " << inclusive_max_size_in_bits);

            const size_t table_size_times_total_TCAM_key_bits = table_size * total_TCAM_key_bits;

            if (table_size_times_total_TCAM_key_bits > inclusive_max_size_in_bits) {
                const size_t max_table_size = inclusive_max_size_in_bits / total_TCAM_key_bits;
                std::stringstream buf;
                buf << "The table '" << table_ptr->name.originalName
                    << "' exceeds the "
                       "maximum size.  It has "
                    << ternary_key_count << " ternary key" << ((1 == ternary_key_count) ? "" : "s")
                    << " of total size " << total_TCAM_key_bits << " bits and a table size of "
                    << table_size << " entries.  The resulting product ("
                    << table_size_times_total_TCAM_key_bits
                    << ") exceeds the maximum supported size (" << inclusive_max_size_in_bits
                    << ") for tables with ternary keys on the current target.  "
                       "This table cannot possibly fit on the target when configured with "
                       "more than "
                    << max_table_size << " entries.";

                /* // safe code
                   // ---------
                std::string local_string = buf.str();
                std::string* heap_string = new std::string(local_string);
                const char* const to_error_with = heap_string->c_str();
                error(to_error_with);
                */
                error(/* slightly risky */ buf.str().c_str() /* slightly risky */);
            }
        }
    }
}

}  // namespace BFN
