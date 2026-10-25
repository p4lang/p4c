// Copyright 2024 Intel Corp.
// SPDX-FileCopyrightText: 2024 Intel Corp.
//
// SPDX-License-Identifier: Apache-2.0

#include "ir/json_generator.h"
#include "ir/json_loader.h"

namespace P4::BitRange {

void rangeToJSON(JSONGenerator &json, int lo, int hi) { json.toJSON(std::make_pair(lo, hi)); }

std::pair<int, int> rangeFromJSON(JSONLoader &json) {
    std::pair<int, int> endpoints;
    json >> endpoints;
    return endpoints;
}

}  // namespace P4::BitRange
