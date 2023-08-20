#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CONTRIB_BMV2_HASH_CALCULATIONS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CONTRIB_BMV2_HASH_CALCULATIONS_H_

/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <cstddef>
#include <cstdint>

namespace P4Tools::P4Testgen::Bmv2 {

class BMv2Hash {
    /* This code was adapted from:
       http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code */

    template <typename T>
    static T reflect(T data, int nBits) {
        T reflection = static_cast<T>(0x00);
        int bit = 0;

        // Reflect the data about the center bit.
        for (bit = 0; bit < nBits; ++bit) {
            // If the LSB bit is set, set the reflection of it.
            if (data & 0x01) {
                reflection |= (static_cast<T>(1) << ((nBits - 1) - bit));
            }
            data = (data >> 1);
        }

        return reflection;
    }

 public:
    static uint16_t crc16(const char *buf, size_t len);

    static uint32_t crc32(const char *buf, size_t len);

    static uint16_t crcCCITT(const char *buf, size_t len);

    static uint16_t csum16(const char *buf, size_t len);

    static uint16_t xor16(const char *buf, size_t len);

    static uint64_t identity(const char *buf, size_t len);
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /*BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CONTRIB_BMV2_HASH_CALCULATIONS_H_*/
