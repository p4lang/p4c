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

#include <bm/bm_sim/core/primitives.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/source_info.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace bm {

namespace core {

REGISTER_PRIMITIVE(assign);

REGISTER_PRIMITIVE(assign_VL);

REGISTER_PRIMITIVE(assign_header);

REGISTER_PRIMITIVE(assign_union);

REGISTER_PRIMITIVE(assign_header_stack);

REGISTER_PRIMITIVE(assign_union_stack);

REGISTER_PRIMITIVE(push);

REGISTER_PRIMITIVE(pop);

REGISTER_PRIMITIVE_W_NAME("assume", assume_);

REGISTER_PRIMITIVE_W_NAME("assert", assert_);

REGISTER_PRIMITIVE_W_NAME("exit", exit_);

REGISTER_PRIMITIVE(log_msg);

void
assign_header::operator ()(Header &dst, const Header &src) {
  if (!src.is_valid()) {
    dst.mark_invalid();
    return;
  }
  dst.mark_valid();
  assert(dst.get_header_type_id() == src.get_header_type_id());
  for (unsigned int i = 0; i < dst.size(); i++) {
    if (dst[i].is_VL())
      dst[i].assign_VL(src[i]);
    else
      dst[i].set(src[i]);
  }
}

void
assign_union::operator ()(HeaderUnion &dst, const HeaderUnion &src) {
  assert(dst.get_num_headers() == src.get_num_headers());
  // naive implementation which iterates over all headers in the union
  for (size_t i = 0; i < dst.get_num_headers(); i++)
    assign_header()(dst.at(i), src.at(i));
}

void
assign_header_stack::operator ()(HeaderStack &dst, const HeaderStack &src) {
  assert(dst.get_depth() == src.get_depth());
  dst.next = src.next;
  for (size_t i = 0; i < dst.get_depth(); i++)
    assign_header()(dst.at(i), src.at(i));
}

void
assign_union_stack::operator ()(HeaderUnionStack &dst,
                                const HeaderUnionStack &src) {
  assert(dst.get_depth() == src.get_depth());
  dst.next = src.next;
  for (size_t i = 0; i < dst.get_depth(); i++)
    assign_union()(dst.at(i), src.at(i));
}

void
push::operator ()(StackIface &stack, const Data &num) {
  stack.push_front(num.get<size_t>());
}

void
pop::operator ()(StackIface &stack, const Data &num) {
  stack.pop_front(num.get<size_t>());
}

void
abort_message(const char *error_msg, const SourceInfo *source_info) {
  if (source_info) {
    BMLOG_ERROR("{}: {} failed, file {}, line {}",
      error_msg, source_info->get_source_fragment(),
      source_info->get_filename(),
      source_info->get_line());
  } else {
    BMLOG_ERROR("{}", error_msg);
  }
  std::abort();
}

#define _BM_COND_ABORT(expr, source_info, error_msg) \
    ((expr) ? abort_message(error_msg, source_info) : (void)0)

void
assert_::operator ()(const Data &src) {
  _BM_COND_ABORT(src.test_eq(0), call_source_info, "Assert error");
}

void
assume_::operator ()(const Data &src) {
  _BM_COND_ABORT(src.test_eq(0), call_source_info, "Assume error");
}

void
exit_::operator ()() {
  get_packet().mark_for_exit();
}

void
log_msg::operator ()(const std::string &format,
                     const std::vector<Data> data_vector) {
  std::string str = format;
  std::string::size_type open = 0;  // position of the opened bracket
  std::string::size_type end = 0;  // position of the matching closed bracket
  unsigned int counter = 0;  // number occurrences of '{}' in string format
  std::string::size_type tmp;

  while ((open = str.find('{', open+1)) != std::string::npos) {
    if (str[open-1] == '\\')
      continue;
    tmp = open;
    while ((end = str.find('}', tmp)) != std::string::npos) {
      if (str[end-1] == '\\') {
        tmp = end+1;
      } else {
        break;
      }
    }
    // TODO(antonin): we could avoid all runtime errors by validation the call
    // to log_msg at runtime.
    if (end == std::string::npos) {
      std::string context = (call_source_info != nullptr) ?
          call_source_info->to_string() : "[no source available]";
      bm::Logger::get()->error(
          "No matching \"}\" in call to log_msg(): {}.", context);
      std::exit(1);
    } else {
      if (counter < data_vector.size()) {
        str.replace(open, end - open + 1,
                    data_vector[counter].get_string_repr());
        open += data_vector[counter++].get_string_repr().size();
      } else {
        counter++;
        open += end - open;
      }
    }
  }

  if (counter != data_vector.size()) {
    std::string context = (call_source_info != nullptr) ?
        call_source_info->to_string() : "[no source available]";
    bm::Logger::get()->error(
        "Not enough arguments for format string in call to log_msg(). "
        "Format string expected ({}) argument(s) but go ({}): {}.",
        counter, data_vector.size(), context);
    std::exit(1);
  }

  tmp = 0;
  while ((tmp = str.find("\\{", tmp)) != std::string::npos) {
    str.erase(tmp, 1);
  }
  tmp = 0;
  while ((tmp = str.find("\\}", tmp)) != std::string::npos) {
    str.erase(tmp, 1);
  }

  bm::Logger::get()->info(str);
}

// dummy method to prevent the removal of global variables (generated by
// REGISTER_PRIMITIVE) by the linker; we call it in the ActionOpcodesMap
// constructor in actions.cpp
int
_bm_core_primitives_import() { return 0; }

}  // namespace core

}  // namespace bm
