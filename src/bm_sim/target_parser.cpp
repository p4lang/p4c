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

#include <bm/bm_sim/target_parser.h>

#include <boost/program_options.hpp>

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <ostream>

namespace po = boost::program_options;

namespace bm {

class TargetParserBasicStore {
  typedef TargetParserBasic::ReturnCode ReturnCode;

 public:
  template <typename T>
  ReturnCode add_option(const std::string &name, const std::string &help_str) {
    if (has_option(name)) return ReturnCode::DUPLICATE_OPTION;
    description.add_options()(name.c_str(), po::value<T>(), help_str.c_str());
    option_added(name);
    return ReturnCode::SUCCESS;
  }

  int parse(const std::vector<std::string> &more_options,
            std::ostream *errstream) {
    po::command_line_parser parser(more_options);
    try {
      po::store(parser.options(description).run(), vm);
      po::notify(vm);
    }
    catch (const std::exception &e) {
      if (errstream) (*errstream) << e.what() << "\n";
      return 1;
    }
    return 0;
  }

  template <typename T>
  ReturnCode get_option(const std::string &name, T *v) const {
    if (!has_option(name)) return ReturnCode::INVALID_OPTION_NAME;
    if (!vm.count(name)) return ReturnCode::OPTION_NOT_PROVIDED;
    try {
      *v = vm.at(name).as<T>();
      return ReturnCode::SUCCESS;
    }
    catch (...) {
      return ReturnCode::INVALID_OPTION_TYPE;
    }
  }

  void get_description(std::ostream *outstream) const {
    (*outstream) << description;
  }

 private:
  bool has_option(const std::string &name) const {
    return option_names.find(name) != option_names.end();
  }

  void option_added(const std::string &name) {
    option_names.insert(name);
  }

  // could not get boost to throw a duplicate_option_error in case of duplicate
  // option names, so I'm using a set to keep track
  std::unordered_set<std::string> option_names;
  po::options_description description{"Target options"};
  po::variables_map vm;
};

template <>
TargetParserBasic::ReturnCode
TargetParserBasicStore::add_option<bool>(const std::string &name,
                                         const std::string &help_str) {
  if (has_option(name)) return ReturnCode::DUPLICATE_OPTION;
  description.add_options()(name.c_str(), help_str.c_str());
  option_added(name);
  return ReturnCode::SUCCESS;
}

template <>
TargetParserBasic::ReturnCode
TargetParserBasicStore::get_option<bool>(const std::string &name,
                                         bool *v) const {
  if (!has_option(name)) return ReturnCode::INVALID_OPTION_NAME;
  *v = static_cast<bool>(vm.count(name));
  return ReturnCode::SUCCESS;
}


TargetParserBasic::TargetParserBasic()
    : var_store(new TargetParserBasicStore()) { }


TargetParserBasic::~TargetParserBasic() { }

int
TargetParserBasic::parse(const std::vector<std::string> &more_options,
                         std::ostream *errstream) {
  return var_store->parse(more_options, errstream);
}

void
TargetParserBasic::help_msg(std::ostream *outstream) const {
  var_store->get_description(outstream);
}

TargetParserBasic::ReturnCode
TargetParserBasic::add_string_option(const std::string &name,
                                     const std::string &help_str) {
  return var_store->add_option<std::string>(name, help_str);
}

TargetParserBasic::ReturnCode
TargetParserBasic::add_int_option(const std::string &name,
                                  const std::string &help_str) {
  return var_store->add_option<int>(name, help_str);
}

TargetParserBasic::ReturnCode
TargetParserBasic::add_flag_option(const std::string &name,
                                   const std::string &help_str) {
  return var_store->add_option<bool>(name, help_str);
}

TargetParserBasic::ReturnCode
TargetParserBasic::get_string_option(const std::string &name,
                                     std::string *v) const {
  return var_store->get_option<std::string>(name, v);
}

TargetParserBasic::ReturnCode
TargetParserBasic::get_int_option(const std::string &name, int *v) const {
  return var_store->get_option<int>(name, v);
}

TargetParserBasic::ReturnCode
TargetParserBasic::get_flag_option(const std::string &name, bool *v) const {
  return var_store->get_option<bool>(name, v);
}

}  // namespace bm
