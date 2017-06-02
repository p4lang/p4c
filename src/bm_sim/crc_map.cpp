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


#include "crc_map.h"

#include <bm/bm_sim/calculations.h>
#include <bm/bm_sim/data.h>

#include <string>

namespace bm {

namespace {

template <typename T>
using crc_config_t = detail::crc_config_t<T>;

template <typename T>
struct CustomCrcName { };

template <>
struct CustomCrcName<uint8_t> {
  static constexpr const char *name = "crc8_custom";
};

template <>
struct CustomCrcName<uint16_t> {
  static constexpr const char *name = "crc16_custom";
};

template <>
struct CustomCrcName<uint32_t> {
  static constexpr const char *name = "crc32_custom";
};

template <>
struct CustomCrcName<uint64_t> {
  static constexpr const char *name = "crc64_custom";
};

template <typename T>
CrcMap::CrcFactoryFn create_crc_factory_fn(const crc_config_t<T> &config) {
  return [config](){
    auto crc = CalculationsMap::get_instance()->get_copy(
        CustomCrcName<T>::name);
    CustomCrcMgr<T>::update_config(crc.get(), config);
    return crc;
  };
}

}  // namespace


// see
// http://crcmod.sourceforge.net/crcmod.predefined.html#predefined-crc-algorithms
// with help from http://protocoltool.sourceforge.net/CRC%20list.html
CrcMap::CrcMap() {
  map_ = {
    {"crc_8", create_crc_factory_fn<uint8_t>({0x07, 0x00, 0x00, false, false})},
    {"crc_8_darc", create_crc_factory_fn<uint8_t>(
      {0x39, 0x00, 0x00, true, true})},
    {"crc_8_i_code", create_crc_factory_fn<uint8_t>(
      {0x1d, 0xfd, 0x00, false, false})},
    {"crc_8_itu", create_crc_factory_fn<uint8_t>(
      {0x07, 0x00, 0x55, false, false})},
    {"crc_8_maxim", create_crc_factory_fn<uint8_t>(
      {0x31, 0x00, 0x00, true, true})},
    {"crc_8_rohc", create_crc_factory_fn<uint8_t>(
      {0x07, 0xff, 0x00, true, true})},
    {"crc_8_wcdma", create_crc_factory_fn<uint8_t>(
      {0x9b, 0x00, 0x00, true, true})},

    {"crc_16", create_crc_factory_fn<uint16_t>(
      {0x8005, 0x0000, 0x0000, true, true})},
    {"crc_16_buypass", create_crc_factory_fn<uint16_t>(
      {0x8005, 0x0000, 0x0000, false, false})},
    {"crc_16_dds_110", create_crc_factory_fn<uint16_t>(
      {0x8005, 0x800d, 0x0000, false, false})},
    {"crc_16_dect", create_crc_factory_fn<uint16_t>(
      {0x0589, 0x0000, 0x0001, false, false})},
    {"crc_16_dnp", create_crc_factory_fn<uint16_t>(
      {0x3d65, 0x0000, 0xffff, true, true})},
    {"crc_16_en_13757", create_crc_factory_fn<uint16_t>(
      {0x3d65, 0x0000, 0xffff, false, false})},
    {"crc_16_genibus", create_crc_factory_fn<uint16_t>(
      {0x1021, 0xffff, 0xffff, false, false})},
    {"crc_16_maxim", create_crc_factory_fn<uint16_t>(
      {0x8005, 0x0000, 0xffff, true, true})},
    {"crc_16_mcrf4xx", create_crc_factory_fn<uint16_t>(
      {0x1021, 0xffff, 0x0000, true, true})},
    {"crc_16_riello", create_crc_factory_fn<uint16_t>(
      {0x1021, 0xb2aa, 0x0000, true, true})},
    {"crc_16_t10_dif", create_crc_factory_fn<uint16_t>(
      {0x8bb7, 0x0000, 0x0000, false, false})},
    {"crc_16_teledisk", create_crc_factory_fn<uint16_t>(
      {0xa097, 0x0000, 0x0000, false, false})},
    {"crc_16_usb", create_crc_factory_fn<uint16_t>(
      {0x8005, 0xffff, 0xffff, true, true})},
    {"x_25", create_crc_factory_fn<uint16_t>(
      {0x1021, 0xffff, 0xffff, true, true})},
    {"xmodem", create_crc_factory_fn<uint16_t>(
      {0x1021, 0x0000, 0x0000, false, false})},
    {"modbus", create_crc_factory_fn<uint16_t>(
      {0x8005, 0xffff, 0x0000, true, true})},
    {"kermit", create_crc_factory_fn<uint16_t>(
      {0x1021, 0x0000, 0x0000, true, true})},
    {"crc_ccitt_false", create_crc_factory_fn<uint16_t>(
      {0x1021, 0xffff, 0x0000, false, false})},
    {"crc_aug_ccitt", create_crc_factory_fn<uint16_t>(
      {0x1021, 0x1d0f, 0x0000, false, false})},

    {"crc_32", create_crc_factory_fn<uint32_t>(
      {0x04c11db7, 0xffffffff, 0xffffffff, true, true})},
    {"crc_32_bzip2", create_crc_factory_fn<uint32_t>(
      {0x04c11db7, 0xffffffff, 0xffffffff, false, false})},
    {"crc_32c", create_crc_factory_fn<uint32_t>(
      {0x1edc6f41, 0xffffffff, 0xffffffff, true, true})},
    {"crc_32d", create_crc_factory_fn<uint32_t>(
      {0xa833982b, 0xffffffff, 0xffffffff, true, true})},
    {"crc_32_mpeg", create_crc_factory_fn<uint32_t>(
      {0x04c11db7, 0xffffffff, 0x00000000, false, false})},
    {"posix", create_crc_factory_fn<uint32_t>(
      {0x04c11db7, 0x00000000, 0xffffffff, false, false})},
    {"crc_32q", create_crc_factory_fn<uint32_t>(
      {0x814141ab, 0x00000000, 0x00000000, false, false})},
    {"jamcrc", create_crc_factory_fn<uint32_t>(
      {0x04c11db7, 0xffffffff, 0x00000000, true, true})},
    {"xfer", create_crc_factory_fn<uint32_t>(
      {0x000000af, 0x00000000, 0x00000000, false, false})},

    {"crc_64", create_crc_factory_fn<uint64_t>(
      {0x000000000000001bULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
       true, true})},
    {"crc_64_we", create_crc_factory_fn<uint64_t>(
      {0x42f0e1eba9ea3693ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
       false, false})},
    {"crc_64_jones", create_crc_factory_fn<uint64_t>(
      {0xad93d23594c935a9ULL, 0xffffffffffffffffULL, 0x0000000000000000ULL,
       true, true})}
  };
}

CrcMap *
CrcMap::get_instance() {
  static CrcMap instance;
  return &instance;
}

std::unique_ptr<CrcMap::MyC>
CrcMap::get_copy(const std::string &name) const {
  auto it = map_.find(name);
  if (it == map_.end()) return nullptr;
  return it->second();
}

namespace {

// 0x1ab -> 8
// 0x112345678 -> 16
// ...
size_t polynomial_order(const std::string &polynomial) {
  auto size = polynomial.size();
  if (size < 3 || polynomial.find("0x1") != 0) return 0;
  return (size - 3) * 4;
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
T hexstr_to_int(const std::string &hexstr) {
  // TODO(antonin): temporary trick to avoid code duplication
  return (hexstr == "") ? 0 : Data(hexstr).get<T>();
}

template <typename T>
std::unique_ptr<CrcMap::MyC>
make_crc_from_custom_params(const std::string &poly, const std::string &init,
                            const std::string &xout, bool data_reflected,
                            bool remainder_reflected) {
  crc_config_t<T> config{
    hexstr_to_int<T>(poly.substr(3)), hexstr_to_int<T>(init),
    hexstr_to_int<T>(xout), data_reflected, remainder_reflected};
  return create_crc_factory_fn<T>(config)();
}

}  // namespace

std::unique_ptr<CrcMap::MyC>
CrcMap::get_copy_from_custom_str(const std::string &str) const {
  std::string::size_type start = 0, pos = 0;
  auto next_token = [&str, &start, &pos]() -> std::string {
    if (pos == std::string::npos) return "";
    pos = str.find('_', start);
    auto token = str.substr(start, pos - start);
    start = pos + 1;
    return token;
  };
  std::string polynomial_str, init_str, xout_str;
  bool data_reflected = true, remainder_reflected = true;
  std::string token;
  while ((token = next_token()) != "") {
    if (token == "poly") {
      polynomial_str = next_token();
      if (polynomial_str == "") return nullptr;
    } else if (token == "init") {
      init_str = next_token();
      if (init_str == "") return nullptr;
    } else if (token == "not" && next_token() == "rev") {
      data_reflected = false;
      remainder_reflected = false;
    } else if (token == "xout") {
      xout_str = next_token();
      if (xout_str == "") return nullptr;
    } else if (token == "lsb") {
      // TODO(antonin)
      return nullptr;
    } else if (token == "msb") {
      // TODO(antonin)
      return nullptr;
    } else if (token == "extend") {
      // TODO(antonin)
      return nullptr;
    } else {  // invalid property
      return nullptr;
    }
  }
  if (polynomial_str == "") return nullptr;  // poly is required
  auto order = polynomial_order(polynomial_str);
  switch (order) {
    case 8:
      return make_crc_from_custom_params<uint8_t>(
          polynomial_str, init_str, xout_str,
          data_reflected, remainder_reflected);
    case 16:
      return make_crc_from_custom_params<uint16_t>(
          polynomial_str, init_str, xout_str,
          data_reflected, remainder_reflected);
    case 32:
      return make_crc_from_custom_params<uint32_t>(
          polynomial_str, init_str, xout_str,
          data_reflected, remainder_reflected);
    case 64:
      return make_crc_from_custom_params<uint64_t>(
          polynomial_str, init_str, xout_str,
          data_reflected, remainder_reflected);
    default:
      return nullptr;
  }
  return nullptr;
}

}  // namespace bm
