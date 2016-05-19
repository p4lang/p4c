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

//! @file target_parser.h
//! This file defines the bm::TargetParserIface interface and the
//! bm::TargetParserBasic class, which implements this interface. Target
//! developpers can write their own parser class, implementing
//! bm::TargetParserIface, and pass an instance of it to
//! bm::SwitchWContexts::init_from_command_line_options, which will be used to
//! parse target-specific options. bm::TargetParserBasic is a simple example of
//! a parser class, which should be enough for many targets.

#ifndef BM_BM_SIM_TARGET_PARSER_H_
#define BM_BM_SIM_TARGET_PARSER_H_

#include <string>
#include <vector>
#include <memory>
#include <ostream>

namespace bm {

//! Interface for target-specific command-line options parsers.
class TargetParserIface {
 public:
  virtual ~TargetParserIface() { }

  //! Called by the bmv2 command-line options parser, when it is done with
  //! general options. All options appearing after `--` will be passed to this
  //! function. \p errstream should be used to log error messages.
  virtual int parse(const std::vector<std::string> &more_options,
                    std::ostream *errstream) = 0;

  //! Called by the bmv2 command-line parser, when the user requests the help
  //! description (with `-h`, `--help`).
  virtual void help_msg(std::ostream *outstream) const = 0;
};

class TargetParserBasicStore;

//! A basic target-specific command-line options parser, which should be enough
//! for many targets.
class TargetParserBasic final : public TargetParserIface {
 public:
  //! Return codes for this class
  enum class ReturnCode {
    //! successful operation
    SUCCESS = 0,
    //! option was already added to the parser
    DUPLICATE_OPTION,
    //! option name not recognized (did not call add)
    INVALID_OPTION_NAME,
    //! the option was not provided by the user
    OPTION_NOT_PROVIDED,
    //! type mismatch, you used the wrong get method
    INVALID_OPTION_TYPE
  };

  //! Constructor
  TargetParserBasic();
  ~TargetParserBasic();

  //! See bm::TargetParserIface::parse. Make sure all possible options have been
  //! registered using add_string_option(), add_int_option() or
  //! add_flag_option().
  int parse(const std::vector<std::string> &more_options,
            std::ostream *errstream) override;

  //! See bm::TargetParserIface::help_msg
  void help_msg(std::ostream *outstream) const override;

  //! Add a command-line option `--<name>` with a value of type string, along
  //! with a help string that will be displayed by help_msg(). You will need to
  //! call get_string_option() to retrieve the value.
  ReturnCode add_string_option(const std::string &name,
                               const std::string &help_str);
  //! Add a command-line option `--<name>` with a value of type int, along with
  //! a help string that will be displayed by help_msg(). You will need to call
  //! get_int_option() to retrieve the value.
  ReturnCode add_int_option(const std::string &name,
                            const std::string &help_str);
  //! Add a command-line flag `--<name>`, along with a help string that will be
  //! displayed by help_msg().You will need to call get_flag_option() to
  //! determine presence / absence of the flag.
  ReturnCode add_flag_option(const std::string &name,
                             const std::string &help_str);

  //! Retrieve the value of command-line option `--<name>`. The option needs to
  //! have been previously registered with add_string_option().
  ReturnCode get_string_option(const std::string &name, std::string *v) const;
  //! Retrieve the value of command-line option `--<name>`. The option needs to
  //! have been previously registered with add_int_option().
  ReturnCode get_int_option(const std::string &name, int *v) const;
  //! Determine whether the command-line flag `--<name>` was present / absent on
  //! the command line.
  ReturnCode get_flag_option(const std::string &name, bool *v) const;

 private:
  std::unique_ptr<TargetParserBasicStore> var_store;
};

}  // namespace bm

#endif  // BM_BM_SIM_TARGET_PARSER_H_
