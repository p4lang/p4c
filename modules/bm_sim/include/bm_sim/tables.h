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

#ifndef _BM_TABLES_H_
#define _BM_TABLES_H_

#include <memory>

#include "control_flow.h"
#include "named_p4object.h"
#include "match_tables.h"
#include "logger.h"

// from http://stackoverflow.com/questions/87372/check-if-a-class-has-a-member-function-of-a-given-signature

template<typename T>
struct HasFactoryMethod
{
  typedef std::unique_ptr<T> (*Signature)(
    const std::string &, const std::string &,
    p4object_id_t, size_t, const MatchKeyBuilder &,
    bool, bool
  );
  template <typename U, Signature> struct SFINAE {};
  template<typename U> static char Test(SFINAE<U, U::create>*);
  template<typename U> static int Test(...);
  static const bool value = sizeof(Test<T>(nullptr)) == sizeof(char);
};

class MatchActionTable : public ControlFlowNode, public NamedP4Object
{
public:
  MatchActionTable(const std::string &name, p4object_id_t id,
		   std::unique_ptr<MatchTableAbstract> match_table)
    : NamedP4Object(name, id),
      match_table(std::move(match_table)) { }

  const ControlFlowNode *operator()(Packet *pkt) const override {
    BMLOG_TRACE_PKT(*pkt, "Applying table '{}'", get_name());
    return match_table->apply_action(pkt);
  }

  MatchTableAbstract *get_match_table() { return match_table.get(); }

public:
  template <typename MT>
  static std::unique_ptr<MatchActionTable> create_match_action_table(
    const std::string &match_type,
    const std::string &name, p4object_id_t id,
    size_t size, const MatchKeyBuilder &match_key_builder,
    bool with_counters, bool with_ageing
  ) {
    static_assert(std::is_base_of<MatchTableAbstract, MT>::value,
		  "incorrect template, needs to be a subclass of MatchTableAbstract");

    static_assert(HasFactoryMethod<MT>::value,
		  "template class needs to have a create() static factory method");

    std::unique_ptr<MT> match_table = MT::create(
      match_type, name, id, size, match_key_builder, with_counters, with_ageing
    );

    return std::unique_ptr<MatchActionTable>(
      new MatchActionTable(name, id, std::move(match_table))
    );
  }

private:
  std::unique_ptr<MatchTableAbstract> match_table;
};

#endif
