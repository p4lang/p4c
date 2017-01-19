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

#include <bm/bm_sim/enums.h>

#include <string>

using namespace bm;

// Google Test fixture for enums tests
class EnumsTest : public ::testing::Test {
 protected:
  EnumMap enums{};

  static const std::string get_name(const std::string &enum_name,
                                    const std::string &entry_name) {
    return enum_name + "." + entry_name;
  }
};

TEST_F(EnumsTest, AddAndAccess) {
  const std::string enum_name("MyEnum");
  const std::string bad_enum_name("BadEnum");
  const std::string enum_entry_1("Entry1");
  const std::string enum_entry_2("Entry2");
  const std::string name_1(get_name(enum_name, enum_entry_1));
  const std::string name_2(get_name(enum_name, enum_entry_2));
  const std::string bad_name("BadName");
  const EnumMap::type_t v_1(0);
  const EnumMap::type_t v_2(1);
  const EnumMap::type_t bad_v(2);

  ASSERT_TRUE(enums.add_enum(enum_name));
  ASSERT_FALSE(enums.add_enum(enum_name));  // error: tries to add twice

  ASSERT_TRUE(enums.add_entry(enum_name, enum_entry_1, v_1));
  // error: tries to add with same name
  ASSERT_FALSE(enums.add_entry(enum_name, enum_entry_1, v_2));
  // error: tries to add with same value
  ASSERT_FALSE(enums.add_entry(enum_name, enum_entry_2, v_1));
  // error: tries to add with invalid enum name
  ASSERT_FALSE(enums.add_entry(bad_enum_name, enum_entry_2, v_2));
  ASSERT_TRUE(enums.add_entry(enum_name, enum_entry_2, v_2));

  ASSERT_EQ(v_1, enums.from_name(name_1));
  ASSERT_EQ(v_2, enums.from_name(name_2));
  ASSERT_THROW(enums.from_name(bad_name), std::out_of_range);

  ASSERT_EQ(name_1, enums.to_name(enum_name, v_1));
  ASSERT_EQ(name_2, enums.to_name(enum_name, v_2));
  ASSERT_THROW(enums.to_name(bad_enum_name, v_1), std::out_of_range);
  ASSERT_THROW(enums.to_name(enum_name, bad_v), std::out_of_range);
}
