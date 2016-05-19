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

#include <vector>
#include <string>

#include <boost/filesystem.hpp>

#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/target_parser.h>

using namespace bm;

namespace fs = boost::filesystem;

namespace {

struct TargetParserDummy : public TargetParserIface {
  int parse(const std::vector<std::string> &more_options,
            std::ostream *errstream) override {
    (void) errstream;
    input = more_options;
    return 0;
  }

  void help_msg(std::ostream *outstream) const override {
    (void) outstream;
  }

  std::vector<std::string> input;
};

struct Argv {
  Argv &push_back(std::string arg) {
    args.push_back(std::move(arg));
    return *this;
  }

  char **get() {
    argv.clear();
    for (const auto &s : args)
      argv.push_back(const_cast<char *>(s.c_str()));
    argv.push_back(nullptr);
    return argv.data();
  }

  const std::vector<std::string> &get_vec() const {
    return args;
  }

  size_t size() const {
    return args.size();
  }

  std::vector<char *> argv;
  std::vector<std::string> args;
};

}  // namespace


class TargetParserIfaceTest : public ::testing::Test {
 protected:
  TargetParserDummy dummy_parser;
  fs::path json_path;

  TargetParserIfaceTest() {
    json_path = fs::path(TESTDATADIR) / fs::path("empty_config.json");
  }
};

TEST_F(TargetParserIfaceTest, NoDispatch) {
  OptionsParser main_parser;
  Argv argv;
  argv.push_back("name")
      .push_back(json_path.string());
  main_parser.parse(argv.size(), argv.get(), nullptr);
  SUCCEED();
}

TEST_F(TargetParserIfaceTest, DispatchEmpty) {
  OptionsParser main_parser;
  Argv argv;
  argv.push_back("name")
      .push_back(json_path.string());
  main_parser.parse(argv.size(), argv.get(), &dummy_parser);
  ASSERT_EQ(0u, dummy_parser.input.size());
}

TEST_F(TargetParserIfaceTest, DispatchOne) {
  OptionsParser main_parser;
  Argv argv;
  argv.push_back("name")
      .push_back(json_path.string())
      .push_back("--")
      .push_back("--unknown").push_back("9");
  main_parser.parse(argv.size(), argv.get(), &dummy_parser);
  ASSERT_EQ(2u, dummy_parser.input.size());
  ASSERT_EQ("--unknown", dummy_parser.input.at(0));
  ASSERT_EQ("9", dummy_parser.input.at(1));
}

TEST_F(TargetParserIfaceTest, DispatchPositional) {
  OptionsParser main_parser;
  Argv argv;
  argv.push_back("name")
      .push_back(json_path.string())
      .push_back("--")
      .push_back("unknown");
  main_parser.parse(argv.size(), argv.get(), &dummy_parser);
  ASSERT_EQ(1u, dummy_parser.input.size());
  ASSERT_EQ("unknown", dummy_parser.input.at(0));
}

TEST_F(TargetParserIfaceTest, DispatchMany) {
  OptionsParser main_parser;
  Argv argv;
  argv.push_back("name")
      .push_back(json_path.string())
      .push_back("--");
  size_t iters = 16;
  for (size_t i = 0; i < iters; i++) {
    argv.push_back("--unknown" + std::to_string(i))
        .push_back(std::to_string(i));
  }
  main_parser.parse(argv.size(), argv.get(), &dummy_parser);
  ASSERT_EQ(2 * iters, dummy_parser.input.size());  
}

class TargetParserBasicTest : public ::testing::Test {
 protected:
  typedef TargetParserBasic::ReturnCode ReturnCode;

  TargetParserBasic target_parser;
  fs::path json_path;

  TargetParserBasicTest() {
    json_path = fs::path(TESTDATADIR) / fs::path("empty_config.json");
  }
};

TEST_F(TargetParserBasicTest, TargetOptionString) {
  const std::string option_name = "option1";
  const std::string strv = "strv";
  std::string v;
  target_parser.add_string_option(option_name, "hs1");
  Argv argv;
  argv.push_back("--" + option_name).push_back(strv);
  target_parser.parse(argv.get_vec(), nullptr);
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.get_string_option(option_name, &v));
  ASSERT_EQ(strv, v);
}

TEST_F(TargetParserBasicTest, TargetOptionInt) {
  const std::string option_name = "option1";
  const int intv = 9;
  int v;
  target_parser.add_int_option(option_name, "hs1");
  Argv argv;
  argv.push_back("--" + option_name).push_back(std::to_string(intv));
  target_parser.parse(argv.get_vec(), nullptr);
  ASSERT_EQ(ReturnCode::SUCCESS, target_parser.get_int_option(option_name, &v));
  ASSERT_EQ(intv, v);
}

TEST_F(TargetParserBasicTest, TargetOptionFlag) {
  const std::string option_name_1 = "option1";
  const std::string option_name_2 = "option2";
  bool v1, v2;
  target_parser.add_flag_option(option_name_1, "hs1");
  target_parser.add_flag_option(option_name_2, "hs2");
  Argv argv;
  argv.push_back("--" + option_name_1);
  target_parser.parse(argv.get_vec(), nullptr);
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.get_flag_option(option_name_1, &v1));
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.get_flag_option(option_name_2, &v2));
  ASSERT_TRUE(v1);
  ASSERT_FALSE(v2);
}

TEST_F(TargetParserBasicTest, TargetOptionMix) {
  target_parser.add_string_option("option1", "hs1");
  target_parser.add_flag_option("option2", "hs2");
  target_parser.add_int_option("option3", "hs3");
  std::string strv;
  int intv;
  bool boolv;
  Argv argv;
  argv.push_back("--option1").push_back("1")
      .push_back("--option2")
      .push_back("--option3").push_back("1");
  target_parser.parse(argv.get_vec(), nullptr);
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.get_string_option("option1", &strv));
  ASSERT_EQ("1", strv);
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.get_flag_option("option2", &boolv));
  ASSERT_TRUE(boolv);
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.get_int_option("option3", &intv));
  ASSERT_EQ(1, intv);
}

TEST_F(TargetParserBasicTest, HelpMsg) {
  // TODO(antonin): worth it to have a better test?
  std::stringstream os;
  ASSERT_EQ("", os.str());
  target_parser.add_string_option("option1", "hs1");
  target_parser.help_msg(&os);
  ASSERT_NE("", os.str());
}

TEST_F(TargetParserBasicTest, DuplicateOption) {
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.add_string_option("option1", "hs1"));
  ASSERT_EQ(ReturnCode::DUPLICATE_OPTION,
            target_parser.add_string_option("option1", "hs1"));
  ASSERT_EQ(ReturnCode::DUPLICATE_OPTION,
            target_parser.add_int_option("option1", "hs1"));
}

TEST_F(TargetParserBasicTest, InvalidOptionName) {
  int v;
  ASSERT_EQ(ReturnCode::INVALID_OPTION_NAME,
            target_parser.get_int_option("unknown", &v));
}

TEST_F(TargetParserBasicTest, InvalidOptionType) {
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.add_string_option("option1", "hs1"));
  Argv argv;
  argv.push_back("--option1").push_back("1");
  target_parser.parse(argv.get_vec(), nullptr);
  int v;
  ASSERT_EQ(ReturnCode::INVALID_OPTION_TYPE,
            target_parser.get_int_option("option1", &v));
}

TEST_F(TargetParserBasicTest, OptionNotProvided) {
  ASSERT_EQ(ReturnCode::SUCCESS,
            target_parser.add_string_option("option1", "hs1"));
  Argv argv;
  target_parser.parse(argv.get_vec(), nullptr);
  std::string v;
  ASSERT_EQ(ReturnCode::OPTION_NOT_PROVIDED,
            target_parser.get_string_option("option1", &v));
}

TEST_F(TargetParserBasicTest, ParsingError) {
  Argv argv;
  argv.push_back("--option1").push_back("1");
  std::stringstream os;
  ASSERT_NE(0, target_parser.parse(argv.get_vec(), &os));
  ASSERT_EQ("unrecognised option '--option1'\n", os.str());
}
