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

#include <netinet/in.h>

#include "bm_sim/calculations.h"

#include "xxhash.h"
#include "crc_tables.h"

namespace hash {

uint64_t xxh64(const char *buffer, size_t s) {
  return XXH64(buffer, s, 0);
}

}

namespace {

/* This code was adapted from:
   http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code */

uint32_t reflect(uint32_t data, int nBits) {
  uint32_t reflection = 0x00000000;
  int bit;

  // Reflect the data about the center bit.
  for (bit = 0; bit < nBits; ++bit) {
    // If the LSB bit is set, set the reflection of it.
    if (data & 0x01) {
      reflection |= (1 << ((nBits - 1) - bit));
    }
    data = (data >> 1);
  }

  return reflection;
}

struct xxh64 {
  uint64_t operator()(const char *buf, size_t len) const {
    return XXH64(buf, len, 0);
  }
};

REGISTER_HASH(xxh64);

struct crc16 {
  uint16_t operator()(const char *buf, size_t len) const {
    uint16_t remainder = 0x0000;
    uint16_t final_xor_value = 0x0000;
    for(unsigned int byte = 0; byte < len; byte++) {
      int data = reflect(buf[byte], 8) ^ (remainder >> 8);
      remainder = table_crc16[data] ^ (remainder << 8);
    }
    /* why is the ntohs() call needed?
       the input buf is made of bytes, yet we return an integer value
       so either I call this function, or I return bytes...
       is returning an integer really the right thing to do?
    */
    uint16_t result = reflect(remainder, 16) ^ final_xor_value;
    return ntohs(result);
  }
};

REGISTER_HASH(crc16);

struct crc32 {
  uint32_t operator()(const char *buf, size_t len) const {
    uint32_t remainder = 0xFFFFFFFF;
    uint32_t final_xor_value = 0xFFFFFFFF;
    for(unsigned int byte = 0; byte < len; byte++) {
      int data = reflect(buf[byte], 8) ^ (remainder >> 24);
      remainder = table_crc16[data] ^ (remainder << 8);
    }
    return ntohl(reflect(remainder, 32) ^ final_xor_value);
  }
};

REGISTER_HASH(crc32);

struct crcCCITT {
  uint16_t operator()(const char *buf, size_t len) const {
    uint16_t remainder = 0xFFFF;
    uint16_t final_xor_value = 0x0000;
    for(unsigned int byte = 0; byte < len; byte++) {
      int data = buf[byte] ^ (remainder >> 8);
      remainder = table_crcCCITT[data] ^ (remainder << 8);
    }
    return ntohs(remainder ^ final_xor_value);
  }
};

REGISTER_HASH(crcCCITT);

struct cksum16 {
  uint16_t operator()(const char *buf, size_t len) const {
    uint64_t sum = 0;
    uint64_t *b = (uint64_t *) buf;
    uint32_t t1, t2;
    uint16_t t3, t4;
    uint8_t *tail;
    /* Main loop - 8 bytes at a time */
    while (len >= sizeof(uint64_t)) {
      uint64_t s = *b++;
      sum += s;
      if (sum < s) sum++;
      len -= 8;
    }
    /* Handle tail less than 8-bytes long */
    tail = (uint8_t *) b;
    if (len & 4) {
      uint32_t s = *(uint32_t *)tail;
      sum += s;
      if (sum < s) sum++;
      tail += 4;
    }
    if (len & 2) {
      uint16_t s = *(uint16_t *) tail;
      sum += s;
      if (sum < s) sum++;
      tail += 2;
    }
    if (len & 1) {
      uint8_t s = *(uint8_t *) tail;
      sum += s;
      if (sum < s) sum++;
    }
    /* Fold down to 16 bits */
    t1 = sum;
    t2 = sum >> 32;
    t1 += t2;
    if (t1 < t2) t1++;
    t3 = t1;
    t4 = t1 >> 16;
    t3 += t4;
    if (t3 < t4) t3++;
    return ntohs(~t3);
  }
};

REGISTER_HASH(cksum16);

struct csum16 {
  uint16_t operator()(const char *buf, size_t len) const {
    return cksum16()(buf, len);
  }
};

REGISTER_HASH(csum16);

}

CalculationsMap * CalculationsMap::get_instance() {
  static CalculationsMap map;
  return &map;
}

bool CalculationsMap::register_one(const char *name, std::unique_ptr<MyC> c) {
  const std::string str_name = std::string(name);
  auto it = map_.find(str_name);
  if(it != map_.end()) return false;
  map_[str_name] = std::move(c);
  return true;
}

std::unique_ptr<CalculationsMap::MyC>
CalculationsMap::get_copy(const std::string &name) {
  auto it = map_.find(name);
  if(it == map_.end()) return nullptr;
  return it->second->clone();
}
