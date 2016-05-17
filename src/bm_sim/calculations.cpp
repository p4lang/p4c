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

#include <bm/bm_sim/calculations.h>
#include <bm/bm_sim/logger.h>

#include <netinet/in.h>

#include <string>
#include <algorithm>
#include <mutex>

#include "xxhash.h"
#include "crc_tables.h"

namespace bm {

namespace hash {

uint64_t xxh64(const char *buffer, size_t s) {
  return XXH64(buffer, s, 0);
}

}  // namespace hash

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

struct crc16 {
  uint16_t operator()(const char *buf, size_t len) const {
    uint16_t remainder = 0x0000;
    uint16_t final_xor_value = 0x0000;
    for (unsigned int byte = 0; byte < len; byte++) {
      int data = reflect(buf[byte], 8) ^ (remainder >> 8);
      remainder = table_crc16[data] ^ (remainder << 8);
    }
    return reflect(remainder, 16) ^ final_xor_value;
  }
};

template <typename T>
struct crc_custom_init { };

template <>
struct crc_custom_init<uint16_t> {
  static uint16_t *crc_table;
  static CustomCrcMgr<uint16_t>::crc_config_t config;
};

uint16_t *crc_custom_init<uint16_t>::crc_table = table_crc16;
CustomCrcMgr<uint16_t>::crc_config_t crc_custom_init<uint16_t>::config =
{0x8005, 0x0000, 0x0000, true, true};

template <>
struct crc_custom_init<uint32_t> {
  static uint32_t *crc_table;
  static CustomCrcMgr<uint32_t>::crc_config_t config;
};

uint32_t *crc_custom_init<uint32_t>::crc_table = table_crc32;
CustomCrcMgr<uint32_t>::crc_config_t crc_custom_init<uint32_t>::config =
{0x04c11db7, 0xffffffff, 0xffffffff, true, true};

template <typename T>
struct crc_custom {
  static constexpr size_t width = sizeof(T) * 8;
  static constexpr size_t kTEntries = 256u;
  typedef typename CustomCrcMgr<T>::crc_config_t crc_config_t;

  crc_custom() {
    std::memcpy(crc_table, crc_custom_init<T>::crc_table, sizeof(crc_table));
    config = crc_custom_init<T>::config;
  }

  T operator()(const char *buf, size_t len) const {
    // clearly not optimized (critical section may be made smaller), but we will
    // try to do better if needed
    std::unique_lock<std::mutex> lock(m);

    T remainder = config.initial_remainder;
    for (unsigned int byte = 0; byte < len; byte++) {
      unsigned char uchar = static_cast<unsigned char>(buf[byte]);
      int data = (config.data_reflected) ?
          reflect(uchar, 8) ^ (remainder >> (width - 8)) :
          uchar ^ (remainder >> (width - 8));
      remainder = crc_table[data] ^ (remainder << 8);
    }
    return (config.remainder_reflected) ?
        reflect(remainder, width) ^ config.final_xor_value :
        remainder ^ config.final_xor_value;
  }

  void update_config(const crc_config_t &new_config) {
    T crc_table_new[kTEntries];
    recompute_crc_table(new_config, crc_table_new);

    std::unique_lock<std::mutex> lock(m);
    config = new_config;
    std::memcpy(crc_table, crc_table_new, sizeof(crc_table));
  }

 private:
  void recompute_crc_table(const crc_config_t &new_config, T *new_table) {
    // Compute the remainder of each possible dividend
    for (size_t dividend = 0; dividend < kTEntries; dividend++) {
      // Start with the dividend followed by zeros
      T remainder = dividend << (width - 8);
      // Perform modulo-2 division, a bit at a time
      for (unsigned char bit = 8; bit > 0; bit--) {
        // Try to divide the current data bit
        if (remainder & (1 << (width - 1))) {
          remainder = (remainder << 1) ^ new_config.polynomial;
        } else {
          remainder = (remainder << 1);
        }
      }

      // Compute the remainder of each possible dividend
      new_table[dividend] = remainder;
    }
  }

  T crc_table[kTEntries];
  crc_config_t config;
  mutable std::mutex m{};
};

struct crc32 {
  uint32_t operator()(const char *buf, size_t len) const {
    uint32_t remainder = 0xFFFFFFFF;
    uint32_t final_xor_value = 0xFFFFFFFF;
    for (unsigned int byte = 0; byte < len; byte++) {
      int data = reflect(buf[byte], 8) ^ (remainder >> 24);
      remainder = table_crc32[data] ^ (remainder << 8);
    }
    return reflect(remainder, 32) ^ final_xor_value;
  }
};

struct crcCCITT {
  uint16_t operator()(const char *buf, size_t len) const {
    uint16_t remainder = 0xFFFF;
    uint16_t final_xor_value = 0x0000;
    for (unsigned int byte = 0; byte < len; byte++) {
      int data = static_cast<unsigned char>(buf[byte]) ^ (remainder >> 8);
      remainder = table_crcCCITT[data] ^ (remainder << 8);
    }
    return remainder ^ final_xor_value;
  }
};

struct cksum16 {
  uint16_t operator()(const char *buf, size_t len) const {
    uint64_t sum = 0;
    const uint64_t *b = reinterpret_cast<const uint64_t *>(buf);
    uint32_t t1, t2;
    uint16_t t3, t4;
    const uint8_t *tail;
    /* Main loop - 8 bytes at a time */
    while (len >= sizeof(uint64_t)) {
      uint64_t s = *b++;
      sum += s;
      if (sum < s) sum++;
      len -= 8;
    }
    /* Handle tail less than 8-bytes long */
    tail = reinterpret_cast<const uint8_t *>(b);
    if (len & 4) {
      uint32_t s = *reinterpret_cast<const uint32_t *>(tail);
      sum += s;
      if (sum < s) sum++;
      tail += 4;
    }
    if (len & 2) {
      uint16_t s = *reinterpret_cast<const uint16_t *>(tail);
      sum += s;
      if (sum < s) sum++;
      tail += 2;
    }
    if (len & 1) {
      uint8_t s = *reinterpret_cast<const uint8_t *>(tail);
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

struct csum16 {
  uint16_t operator()(const char *buf, size_t len) const {
    return cksum16()(buf, len);
  }
};

struct identity {
  uint64_t operator()(const char *buf, size_t len) const {
    uint64_t res = 0ULL;
    for (size_t i = 0; i < std::min(sizeof(res), len); i++) {
      if (i > 0) res <<= 8;
      res += static_cast<uint8_t>(buf[i]);
    }
    return res;
  }
};

}  // namespace

// if REGISTER_HASH calls placed in the anonymous namespace, some compiler can
// give an unused variable warning
REGISTER_HASH(xxh64);
REGISTER_HASH(crc16);
REGISTER_HASH(crc32);
REGISTER_HASH(crcCCITT);
REGISTER_HASH(cksum16);
REGISTER_HASH(csum16);
REGISTER_HASH(identity);

typedef crc_custom<uint16_t> crc16_custom;
REGISTER_HASH(crc16_custom);
typedef crc_custom<uint32_t> crc32_custom;
REGISTER_HASH(crc32_custom);

template <typename T>
CustomCrcErrorCode
CustomCrcMgr<T>::update_config(NamedCalculation *calculation,
                               const crc_config_t &config) {
  Logger::get()->info("Updating config of custom crc {}: {}",
                      calculation->get_name(), config);
  auto raw_c_iface = calculation->get_raw_calculation();
  return update_config(raw_c_iface, config);
}

template <typename T>
CustomCrcErrorCode
CustomCrcMgr<T>::update_config(RawCalculationIface<uint64_t> *c,
                               const crc_config_t &config) {
  typedef RawCalculation<uint64_t, crc_custom<T> > ExpectedCType;
  auto raw_c = dynamic_cast<ExpectedCType *>(c);
  if (!raw_c) return CustomCrcErrorCode::WRONG_TYPE_CALCULATION;
  raw_c->get_hash_fn().update_config(config);
  return CustomCrcErrorCode::SUCCESS;
}

// explicit instantiation, should prevent implicit instantiation
template class CustomCrcMgr<uint16_t>;
template class CustomCrcMgr<uint32_t>;

CalculationsMap * CalculationsMap::get_instance() {
  static CalculationsMap map;
  return &map;
}

bool CalculationsMap::register_one(const char *name, std::unique_ptr<MyC> c) {
  const std::string str_name = std::string(name);
  auto it = map_.find(str_name);
  if (it != map_.end()) return false;
  map_[str_name] = std::move(c);
  return true;
}

std::unique_ptr<CalculationsMap::MyC>
CalculationsMap::get_copy(const std::string &name) {
  auto it = map_.find(name);
  if (it == map_.end()) return nullptr;
  return it->second->clone();
}

}  // namespace bm
