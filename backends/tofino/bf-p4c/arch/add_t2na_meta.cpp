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

#include "bf-p4c/arch/add_t2na_meta.h"

namespace BFN {

// Check T2NA metadata structures and headers and add missing fields
void AddT2naMeta::postorder(IR::Type_StructLike *typeStructLike) {
    cstring typeStructLikeName = typeStructLike->name;
    if (typeStructLikeName == "ingress_intrinsic_metadata_for_deparser_t" ||
        typeStructLikeName == "egress_intrinsic_metadata_for_deparser_t") {
        if (typeStructLike->fields.getDeclaration("mirror_io_select"_cs)) {
            LOG3("AddT2naMeta : " << typeStructLikeName << " already complete");
            return;
        }
        LOG3("AddT2naMeta : Adding missing fields to " << typeStructLikeName);
        typeStructLike->fields.push_back(
            new IR::StructField("mirror_io_select", IR::Type::Bits::get(1)));
        typeStructLike->fields.push_back(
            new IR::StructField("mirror_hash", IR::Type::Bits::get(13)));
        typeStructLike->fields.push_back(
            new IR::StructField("mirror_ingress_cos", IR::Type::Bits::get(3)));
        typeStructLike->fields.push_back(
            new IR::StructField("mirror_deflect_on_drop", IR::Type::Bits::get(1)));
        typeStructLike->fields.push_back(
            new IR::StructField("mirror_multicast_ctrl", IR::Type::Bits::get(1)));
        typeStructLike->fields.push_back(
            new IR::StructField("mirror_egress_port", new IR::Type_Name("PortId_t")));
        typeStructLike->fields.push_back(
            new IR::StructField("mirror_qid", new IR::Type_Name("QueueId_t")));
        typeStructLike->fields.push_back(
            new IR::StructField("mirror_coalesce_length", IR::Type::Bits::get(8)));
        typeStructLike->fields.push_back(
            new IR::StructField("adv_flow_ctl", IR::Type::Bits::get(32)));
        typeStructLike->fields.push_back(
            new IR::StructField("mtu_trunc_len", IR::Type::Bits::get(14)));
        typeStructLike->fields.push_back(
            new IR::StructField("mtu_trunc_err_f", IR::Type::Bits::get(1)));
        if (typeStructLikeName == "ingress_intrinsic_metadata_for_deparser_t") {
            typeStructLike->fields.push_back(new IR::StructField("pktgen", IR::Type::Bits::get(1)));
            typeStructLike->fields.push_back(
                new IR::StructField("pktgen_address", IR::Type::Bits::get(14)));
            typeStructLike->fields.push_back(
                new IR::StructField("pktgen_length", IR::Type::Bits::get(10)));
        }
    } else if (typeStructLikeName == "egress_intrinsic_metadata_t") {
        const IR::StructField *lastStructField = nullptr;
        for (const auto *structField : typeStructLike->fields) {
            lastStructField = structField;
        };
        if (lastStructField && lastStructField->annotations->getSingle("padding"_cs)) {
            LOG3("AddT2naMeta : " << typeStructLikeName << " already complete");
            return;
        }
        LOG3("AddT2naMeta : Adding missing fields to " << typeStructLikeName);
        typeStructLike->fields.push_back(new IR::StructField(
            "_pad_eg_int_md", new IR::Annotations({new IR::Annotation("padding", {})}),
            IR::Type::Bits::get(8)));
    }
}

}  // namespace BFN
