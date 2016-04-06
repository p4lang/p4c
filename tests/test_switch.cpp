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

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>

#include "bm_sim/switch.h"

using namespace bm;

namespace fs = boost::filesystem;

namespace {

char char2digit(char c) {
  if (c >= '0' && c <= '9')
    return (c - '0');
  if (c >= 'A' && c <= 'F')
    return (c - 'A' + 10);
  if (c >= 'a' && c <= 'f')
    return (c - 'a' + 10);
  assert(0);
  return 0;
}

}  // namespace

class SwitchTest : public Switch {
  int receive(int port_num, const char *buffer, int len) override {
    (void) port_num; (void) buffer; (void) len;
    return 0;
  }

  void start_and_return() override {
  }
};

#include <iostream>

TEST(Switch, GetConfig) {
  fs::path config_path = fs::path(TESTDATADIR) / fs::path("empty_config.json");
  // the md5 is that file was computed with md5sum on Ubuntu
  fs::path md5_path = fs::path(TESTDATADIR) / fs::path("empty_config.md5");
  std::stringstream config_buffer;
  std::vector<char> md5(16);
  {
    std::ifstream fs(config_path.string());
    config_buffer << fs.rdbuf();
  }
  {
    std::ifstream fs(md5_path.string());
    for (size_t i = 0; i < md5.size(); i++) {
      char c1, c2;
      assert(fs.get(c1)); assert(fs.get(c2));
      md5[i] = (char2digit(c1) << 4) | char2digit(c2);
    }
  }
  SwitchTest sw;
  sw.init_objects(config_path.string(), 0, nullptr);

  ASSERT_EQ(config_buffer.str(), sw.get_config());

  ASSERT_EQ(std::string(md5.begin(), md5.end()), sw.get_config_md5());
}
