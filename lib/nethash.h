#ifndef LIB_NETHASH_H_
#define LIB_NETHASH_H_

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
#include <vector>

/// A collection of hashing functions commonly used in network protocols.
namespace nethash {

uint16_t crc16(const uint8_t *buf, size_t len);
uint32_t crc32(const uint8_t *buf, size_t len);
uint16_t crcCCITT(const uint8_t *buf, size_t len);
uint16_t csum16(const uint8_t *buf, size_t len);
uint16_t xor16(const uint8_t *buf, size_t len);
uint64_t identity(const uint8_t *buf, size_t len);

}  // namespace nethash

#endif /* LIB_NETHASH_H_ */
