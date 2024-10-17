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

#include "bf-p4c/ir/bitrange.h"

#include <iostream>
#include <utility>

#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"

void rangeToJSON(P4::JSONGenerator& json, int lo, int hi) {
    json.toJSON(std::make_pair(lo, hi));
}

std::pair<int, int> rangeFromJSON(P4::JSONLoader& json) {
    std::pair<int, int> endpoints;
    json >> endpoints;
    return endpoints;
}
