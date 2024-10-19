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

#include "bf-p4c/common/run_id.h"

#include <iomanip>
#include <sstream>

#include <boost/uuid/random_generator.hpp>

RunId::RunId() {
    auto uuid = boost::uuids::random_generator()();
    auto uuidBytes = std::vector<uint8_t>(uuid.begin(), uuid.end());
    std::stringstream ss;
    ss << std::hex;
    for (uint8_t b : uuidBytes) {
        ss << std::setw(2) << std::setfill('0') << (int)b;
    }
    _runId = ss.str();
    // Take only the first 16 characters to keep backward-compatibility with previous
    // implementations.
    _runId.resize(16);
}
