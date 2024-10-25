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

#include "compute_buffer_requirements.h"

#include "bf-p4c/device.h"

namespace Parde::Lowered {

void ComputeBufferRequirements::postorder(IR::BFN::LoweredParserMatch *match) {
    // Determine the range of bytes in the input packet read if this match
    // is successful. Note that we ignore `LoweredBufferRVal`s and
    // `LoweredConstantRVal`s, since those do not originate in the input
    // packet.
    nw_byteinterval bytesRead;
    forAllMatching<IR::BFN::LoweredExtractPhv>(
        &match->extracts, [&](const IR::BFN::LoweredExtractPhv *extract) {
            if (auto *source = extract->source->to<IR::BFN::LoweredPacketRVal>()) {
                bytesRead = bytesRead.unionWith(source->byteInterval());
            }
        });

    forAllMatching<IR::BFN::LoweredSave>(&match->saves, [&](const IR::BFN::LoweredSave *save) {
        if (save->source->byteInterval().hiByte() < Device::pardeSpec().byteInputBufferSize())
            bytesRead = bytesRead.unionWith(save->source->byteInterval());
    });

    forAllMatching<IR::BFN::LoweredParserChecksum>(
        &match->checksums, [&](const IR::BFN::LoweredParserChecksum *cks) {
            for (const auto &r : cks->masked_ranges) {
                bytesRead = bytesRead.unionWith(toHalfOpenRange(r));
            }
        });

    // We need to have buffered enough bytes to read the last byte in the
    // range. We also need to be sure to buffer at least as many bytes as we
    // plan to shift.
    match->bufferRequired = std::max(unsigned(bytesRead.hi), match->shift);

    auto state = findContext<IR::BFN::LoweredParserState>();

    const unsigned inputBufferSize = Device::pardeSpec().byteInputBufferSize();
    BUG_CHECK(*match->bufferRequired <= inputBufferSize,
              "Parser state %1% requires %2% bytes to be buffered which "
              "is greater than the size of the input buffer (%3% byte)",
              state->name, *match->bufferRequired, inputBufferSize);
}

}  // namespace Parde::Lowered
