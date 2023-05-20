#include "backends/tc/ir_builder.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <numeric>
#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "backends/tc/instruction.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/util.h"
#include "ir/ir-generated.h"
#include "lib/gmputil.h"
#include "lib/log.h"
#include "lib/null.h"

namespace backends::tc {

namespace {

// The in-state for a given P4 state. Does not mangle the names of special
// states.
State InState(State state) {
  if (IsSpecial(state)) {
    return state;
  } else {
    return state + "_in";
  }
}

// The out-state for a given P4 state. Does not mangle the names of special
// states except start.
State OutState(State state) {
  if (IsSpecial(state) && state != kStartState) {
    return state;
  } else {
    return state + "_out";
  }
}

}  // namespace

std::vector<std::pair<std::string, size_t>> IRBuilder::ExtractHeaderFields(
    const std::string &prefix, const IR::Type_StructLike *header_type) {
  // We handle only struct-like types and bits
  std::vector<std::pair<std::string, size_t>> field_to_size;
  for (const auto declaration : *header_type->getDeclarations()) {
    const auto field = declaration->as<const IR::StructField &>();

    if (const auto struct_like =
            dynamic_cast<const IR::Type_StructLike *>(field.type)) {
      auto subfields = this->ExtractHeaderFields(
          prefix + field.name.toString() + ".", struct_like);
      // move the subfields to field_to_size
      const auto old_size = field_to_size.size();
      field_to_size.resize(old_size + subfields.size());
      std::move(subfields.begin(), subfields.end(),
                field_to_size.begin() + old_size);
    } else if (const auto bits =
                   dynamic_cast<const IR::Type_Bits *>(field.type)) {
      field_to_size.push_back({prefix + field.name.toString(), bits->size});
    } else {
      // We handle only headers and sized primitives
      UnrecoverableErrorAt(field.getSourceInfo(), "Cannot handle the type '",
                           field.type->toString(),
                           "' in header/struct declaration. Only "
                           "bits<N> and headers/structs are supported inside "
                           "extracted headers.");
    }
  }
  return field_to_size;
}

absl::optional<std::vector<IR::ID>> IRBuilder::ResolveHeaderExpression(
    const IR::Expression *expr) const {
  // We traverse the expression to build the vector backwards (inserting the
  // outmost field access first), then reverse it.
  std::vector<IR::ID> field_accesses;
  while (auto member_expr = dynamic_cast<const IR::Member *>(expr)) {
    field_accesses.push_back(member_expr->member);
    expr = member_expr->expr;
  }
  if (auto root_expr = dynamic_cast<const IR::PathExpression *>(expr)) {
    auto root_decl = ref_map_.getDeclaration(root_expr->path);
    if (root_decl->getName() == *this->header_param_) {
      std::reverse(field_accesses.begin(), field_accesses.end());
      return field_accesses;
    }
  }

  // This is not a valid access to a header field we can reason about during
  // compile time.
  return {};
}

std::vector<util::Range> IRBuilder::GetRangesRelativeToCursor(
    const std::string &current_state,
    const IR::ListExpression *select_guard) const {
  std::vector<util::Range> ranges;
  const auto &code_gen_info = code_gen_info_.at(current_state);
  for (const auto &expr : select_guard->components) {
    // Extract the header field for each expression, then get the range of the
    // field.
    if (const auto header_field = ResolveHeaderExpression(expr)) {
      // Get the flattened field path `hdr.field1.field2`
      std::string field_path = header_field->front().name.c_str();
      for (auto id = header_field->begin() + 1; id != header_field->end();
           ++id) {
        field_path += ".";
        field_path += id->name.c_str();
      }

      if (const auto field_offset_and_size =
              tcam_program_.FieldOffsetAndSize(field_path)) {
        auto header_name = header_field->front().name.c_str();
        auto header_offset = code_gen_info.header_offsets.find(header_name);
        if (header_offset == code_gen_info.header_offsets.end()) {
          UnrecoverableErrorAt(expr->getSourceInfo(), "The header '",
                               header_name,
                               "' is not extracted in the current state.");
        }
        const auto absolute_offset =
            header_offset->second + field_offset_and_size->first;
        const auto field_size = field_offset_and_size->second;
        ranges.push_back(
            util::Range::Create(absolute_offset, absolute_offset + field_size)
                .value());
      } else {
        UnrecoverableErrorAt(expr->getSourceInfo(),
                             "The header field is not defined.");
      }
    } else if (const auto path =
                   dynamic_cast<const IR::PathExpression *>(expr)) {
      const auto var = ref_map_.getDeclaration(path->path);
      if (code_gen_info.available_locals.contains(var)) {
        ranges.push_back(code_gen_info.available_locals.at(var));
      } else {
        UnrecoverableErrorAt(expr->getSourceInfo(),
                             "Only the variables computed in this state are "
                             "allowed to be used in a select expression.");
      }
    } else if (const auto method_call =
                   dynamic_cast<const IR::MethodCallExpression *>(expr)) {
      if (const auto method =
              dynamic_cast<const IR::Member *>(method_call->method)) {
        auto method_name = method->member;
        if (method_name == this->kLookahead) {
          // Lookahead reads bits from the current offset forward
          const auto current_offset = code_gen_info.current_offset;
          const size_t size = method_call->typeArguments->at(0)->width_bits();
          ranges.push_back(
              util::Range::Create(current_offset, current_offset + size)
                  .value());
        } else {
          UnrecoverableErrorAt(
              expr->getSourceInfo(),
              "Call to a method other than packet.lookahead.Expected a header "
              "field or a call to packet.lookahead.");
        }
      } else {
        UnrecoverableErrorAt(expr->getSourceInfo(),
                             "Expected a header field or a call to "
                             "packet.lookahead. Found different method call.");
      }
    } else {
      UnrecoverableErrorAt(
          expr->getSourceInfo(),
          "Expected a header field or a call to packet.lookahead. Found ",
          expr->node_type_name());
    }
  }

  return ranges;
}

// The visitor for the parser. It collects the headers used by the parser, and
// generates header declarations.
bool IRBuilder::preorder(const IR::P4Parser *parser) {
  if (!parser->getTypeParameters()->empty()) {
    ReportErrorAt(
        parser->getTypeParameters()->getSourceInfo(),
        "Type parameters are not supported. Parser '%s' uses type parameters.");
    // stop code generation for this parser here
    return false;
  }

  // Get the header types from the type of the second parameter
  auto header_param = parser->getApplyParameters()->getParameter(1);
  if (!header_param || header_param->direction != IR::Direction::Out) {
    ReportErrorAt(
        parser->getApplyParameters()->getSourceInfo(),
        "The second parameter of the parser should be an out parameter that "
        "carries the extracted headers.");
    return false;
  }

  this->header_param_ = header_param->getName();

  const auto header_struct = dynamic_cast<const IR::Type_StructLike *>(
      type_map_.getType(header_param));

  if (!header_struct) {
    ReportErrorAt(
        header_param->getSourceInfo(),
        "The type of the extracted headers should be struct-like, but ",
        header_param->getName().toString(), " has type ",
        type_map_.getType(header_param)->toString());
    return false;
  }

  // Extract the declared headers
  for (const auto declaration : *header_struct->getDeclarations()) {
    const auto field = declaration->as<const IR::StructField &>();
    // All fields must be headers
    const auto field_type = dynamic_cast<const IR::Type_Header *>(field.type);
    CHECK_NULL(field_type);
    // Extract the top-level field type, then associate this field with that
    // type.
    const auto extracted_fields = ExtractHeaderFields("", field_type);
    tcam_program_.header_type_definitions_.emplace(
        field_type->name.toString().c_str(), std::move(extracted_fields));
    tcam_program_.header_instantiations_.emplace(
        field.name.toString().c_str(), field_type->name.toString().c_str());
  }

  return true;
}

bool IRBuilder::preorder(const IR::ParserState *state) {
  if (!this->header_param_) {
    ::error(
        "Internal compiler error: header parameter is not set but started "
        "processing parser states");
    throw IRBuilderError{};
  }

  this->code_gen_info_.emplace(state->name.toString().c_str(), CodeGenInfo{});

  // visit the components
  return true;
}

bool IRBuilder::preorder(const IR::Statement *statement) {
  auto state = findContext<IR::ParserState>();
  if (!state) {
    // This statement is outside a parser state, so just skip it
    return false;
  }
  auto &code_gen_info = code_gen_info_.at(state->name.toString().c_str());

  // Rather than specialized methods with boilerplate for each statement kind,
  // match them using dynamic_cast
  if (auto method_call =
          dynamic_cast<const IR::MethodCallStatement *>(statement)) {
    if (auto method =
            dynamic_cast<const IR::Member *>(method_call->methodCall->method)) {
      auto method_name = method->member;
      if (kSupportedMethods.contains(method_name.name.c_str())) {
        auto implementation = kSupportedMethods.at(method_name.name.c_str());
        if (!implementation(*this, method_call)) {
          // Translation of this method failed, do not translate child nodes
          return false;
        }
      } else {
        ReportErrorAt(method_name.getSourceInfo(), "The method '",
                      method_name.originalName,
                      "' is not supported by this backend.");
        return false;
      }
    } else {
      ReportErrorAt(method_call->methodCall->method->getSourceInfo(),
                    "The callee of method call is not an access path "
                    "(something like packet.extract).");
      return false;
    }
  } else if (dynamic_cast<const IR::EmptyStatement *>(statement)) {
    // Do nothing
  } else if (dynamic_cast<const IR::BlockStatement *>(statement)) {
    // We can generate code for block statements as long as they are not part of
    // a control-flow statement. So, visit children.
    return true;
  } else if (const auto assignment =
                 dynamic_cast<const IR::AssignmentStatement *>(statement)) {
    if (const auto lhs =
            dynamic_cast<const IR::PathExpression *>(assignment->left)) {
      const auto local = ref_map_.getDeclaration(lhs->path);
      bool extracted_rhs = false;
      if (const auto method_call =
              dynamic_cast<const IR::MethodCallExpression *>(
                  assignment->right)) {
        if (const auto method =
                dynamic_cast<const IR::Member *>(method_call->method)) {
          auto method_name = method->member;
          if (method_name == this->kLookahead) {
            // Lookahead reads bits from the current offset forward
            const auto current_offset = code_gen_info.current_offset;
            const size_t size = method_call->typeArguments->at(0)->width_bits();
            code_gen_info.available_locals.emplace(
                local,
                util::Range::Create(current_offset, current_offset + size)
                    .value());
            extracted_rhs = true;
          }
        }
      } else if (const auto path = dynamic_cast<const IR::PathExpression *>(
                     assignment->right)) {
        const auto rhs_local = ref_map_.getDeclaration(path->path);
        if (code_gen_info.available_locals.contains(rhs_local)) {
          code_gen_info.available_locals.emplace(
              local, util::Range(code_gen_info.available_locals.at(rhs_local)));
          extracted_rhs = true;
        }
      }
      if (!extracted_rhs) {
        ReportErrorAt(
            statement->getSourceInfo(),
            "Right hand side of an assignment should be either a previously "
            "assigned variable or a call to packet.lookahead<T>");
        return false;
      }
    } else {
      ReportErrorAt(statement->getSourceInfo(),
                    "Can assign to variables only.");
      return false;
    }
  } else {
    ReportErrorAt(statement->getSourceInfo(), "Statements of type ",
                  statement->node_type_name(), " are not supported.");
    return false;
  }

  // We do not need to visit child nodes with a visitor method, because we
  // already processed the current statement, and we do not support any nested
  // statements like control flow statements.
  return false;
}

bool IRBuilder::preorder(const IR::PathExpression *expression) {
  auto state = findContext<IR::ParserState>();
  // Check if this is the expression used for state transition.
  if (!state || state->selectExpression != expression) {
    return false;
  }
  auto state_in = OutState(state->name.toString().c_str());
  auto state_out = OutState(state->name.toString().c_str());
  auto next_state_in = InState(expression->path->name.toString().c_str());

  // No need to generate set-key instructions, just forward state_out to
  // next_state_in
  this->tcam_program_.FindOrInsertTCAMEntry(state_in, {}, {})
      .instructions.push_back(std::make_shared<NextState>(state_out));
  this->tcam_program_.FindOrInsertTCAMEntry(state_out, {}, {})
      .instructions.push_back(std::make_shared<NextState>(next_state_in));
  return false;
}

absl::optional<std::vector<std::pair<Value, Mask>>>
IRBuilder::CollectValuesAndMasks(
    const IR::Expression *keyset_expression) const {
  using ValuesAndMasks = std::vector<std::pair<Value, Mask>>;

  // Read the big int, and convert it to an MSB-first bitvector of given size,
  // zero-extend if `n` has less than `size` bits.
  auto big_int_to_bitvector = [](size_t size, const big_int &n) {
    std::vector<bool> result(size, false);
    // set the bits of the value, MSB-first
    for (size_t i = 0; i < size; ++i) {
      result[size - i - 1] = boost::multiprecision::bit_test(n, i);
    }
    return result;
  };

  if (const auto constant =
          dynamic_cast<const IR::Constant *>(keyset_expression)) {
    // It is a single constant with no mask, get its width from the type
    size_t size = constant->type->width_bits();
    std::vector<bool> mask(size, true);  // The mask has all bits set
    auto value = big_int_to_bitvector(size, constant->value);
    return ValuesAndMasks{std::pair<Value, Mask>{value, mask}};
  } else if (const auto _ = dynamic_cast<const IR::DefaultExpression *>(
                 keyset_expression)) {
    // This is the default case, return an empty mask and value because the type
    // information for keyset_expression does not carry size for `default`.
    return ValuesAndMasks{std::pair<Value, Mask>{{}, {}}};
  } else if (const auto mask_expr =
                 dynamic_cast<const IR::Mask *>(keyset_expression)) {
    // Both sides of the mask should be constant
    if (const auto lhs = dynamic_cast<const IR::Constant *>(mask_expr->left)) {
      if (const auto rhs =
              dynamic_cast<const IR::Constant *>(mask_expr->right)) {
        // Use the mask's size
        size_t size = rhs->type->width_bits();
        auto value = big_int_to_bitvector(size, lhs->value);
        auto mask = big_int_to_bitvector(size, rhs->value);
        return ValuesAndMasks{std::pair<Value, Mask>{value, mask}};
      } else {
        ReportErrorAt(mask_expr->right->getSourceInfo(),
                      "Right-hand side of mask is not a constant");
        return absl::nullopt;
      }
    } else {
      ReportErrorAt(mask_expr->left->getSourceInfo(),
                    "Left-hand side of mask is not a constant");
      return absl::nullopt;
    }
  } else if (const auto tuple =
                 dynamic_cast<const IR::ListExpression *>(keyset_expression)) {
    std::vector<std::pair<std::vector<bool>, std::vector<bool>>>
        values_and_masks;
    for (const IR::Expression *tuple_field : tuple->components) {
      if (auto values_from_tuple_field = CollectValuesAndMasks(tuple_field)) {
        for (auto &&value_and_mask : *values_from_tuple_field) {
          values_and_masks.push_back(value_and_mask);
        }
      } else {
        // Propagate failure
        return absl::nullopt;
      }
    }
    return values_and_masks;
  } else {
    ReportErrorAt(keyset_expression->getSourceInfo(),
                  "Keyset expressions of type ",
                  keyset_expression->node_type_name(),
                  " are not supported by this backend.");
    return absl::nullopt;
  }
}

bool IRBuilder::preorder(const IR::SelectExpression *expression) {
  auto state = findContext<IR::ParserState>();
  // Check if this is the expression used for state transition.
  if (!state || state->selectExpression != expression) {
    return false;
  }

  const auto field_ranges = this->GetRangesRelativeToCursor(
      state->name.name.c_str(), expression->select);
  const size_t begin_offset =
      std::min_element(field_ranges.begin(), field_ranges.end(),
                       [](const util::Range &lhs, const util::Range &rhs) {
                         return lhs.from() < rhs.from();
                       })
          ->from();
  const size_t end_offset =
      std::max_element(field_ranges.begin(), field_ranges.end(),
                       [](const util::Range &lhs, const util::Range &rhs) {
                         return lhs.to() < rhs.to();
                       })
          ->to();

  auto state_in = InState(state->name.name.c_str());
  auto state_out = OutState(state->name.name.c_str());

  // Generate the instructions to transition from state_in to state_out
  auto &state_in_instructions =
      tcam_program_.FindOrInsertTCAMEntry(state_in, {}, {}).instructions;
  state_in_instructions.push_back(std::make_shared<SetKey>(
      util::Range::Create(begin_offset, end_offset).value()));
  state_in_instructions.push_back(std::make_shared<NextState>(state_out));

  // A vector of select cases seen so far, along with the length of the prefix
  // covered by the mask. The fields of the tuple are
  //
  // (prefix length, value, mask).
  //
  // It is used for checking that the semantics of the P4 parser (first
  // switch that matches) and the semantics of the TCAM (longest prefix matches)
  // are the same for this select statement.
  std::vector<std::tuple<size_t, std::vector<bool>, std::vector<bool>>>
      previous_cases;
  for (const auto &select_case : expression->selectCases) {
    // Initialize the value and the mask to be all 0s, we will fill them as we
    // process the keyset expression
    std::vector<bool> value(end_offset - begin_offset, false);
    std::vector<bool> mask(end_offset - begin_offset, false);
    const auto next_state_in =
        InState(select_case->state->path->name.toString().c_str());
    if (auto field_values_and_masks =
            CollectValuesAndMasks(select_case->keyset)) {
      // Edge case for when the case is `default` and select is over a tuple. In
      // this case, CollectValuesAndMasks returns a single pair of empty value
      // and mask, add more empty values and masks to match the tuple in the
      // select expression.
      if (field_values_and_masks->size() == 1 && field_ranges.size() > 1) {
        for (size_t i = 1; i < field_ranges.size(); ++i) {
          field_values_and_masks->push_back({{}, {}});
        }
      }

      // At this point field_values_and_masks contains one entry per tuple field
      // in field_ranges. Each entry in field_values_and_masks should be in one
      // of the two forms:
      //
      // - A pair of value and mask where the size matches the size of the
      // corresponding field.
      //
      // - An empty value and mask denoting the default case.
      //
      // Go through each tuple element, find the correct range, and set the
      // value and mask accordingly.
      for (size_t i = 0; i < field_ranges.size(); ++i) {
        const auto &field_range = field_ranges.at(i);
        auto &field_value = field_values_and_masks->at(i).first;
        auto &field_mask = field_values_and_masks->at(i).second;
        if (field_mask.size() != field_range.size()) {
          if (field_mask.empty()) {
            // This is a default case, update field_value and field_mask to
            // match the size of field_range.
            field_value = Value(field_range.size(), false);
            field_mask = Mask(field_range.size(), false);
          } else {
            ReportErrorAt(
                select_case->keyset->getSourceInfo(),
                "The value or the mask for the tuple element #", i,
                " in this select case and the select expression \n",
                expression->select->getSourceInfo().toSourceFragment(),
                "\n have different bitwidth (", field_range.size(), " bits vs ",
                field_mask.size(),
                " bits"
                "). The extracted value and mask are ",
                util::BitStringToString(field_value), " and ",
                util::BitStringToString(field_mask),
                " respectively, and the tuple element matches the range ",
                field_range, " after the cursor");
            return false;
          }
        }
        const auto field_offset_relative_to_key =
            field_range.from() - begin_offset;
        std::copy_n(field_value.begin(), field_range.size(),
                    value.begin() + field_offset_relative_to_key);
        std::copy_n(field_mask.begin(), field_range.size(),
                    mask.begin() + field_offset_relative_to_key);
      }
    } else {
      // Failed to extract values and masks from the keyset expression.
      return false;
    }

    // Create the TCAM entry for this case
    tcam_program_.InsertTCAMEntry(state_out, value, mask)
        .instructions.push_back(std::make_shared<NextState>(next_state_in));

    // Check if this case overlaps any of the previous cases
    auto lsb = std::find(mask.rbegin(), mask.rend(), true);
    // The prefix is 0 bits if the mask is all 0s, otherwise its length is the
    // position of the least-significant bit + 1.
    auto prefix_length = mask.size() - (lsb - mask.rbegin());
    for (const auto &previous_case : previous_cases) {
      // Use structured binding when switching to C++17
      auto &prev_prefix_length = std::get<0>(previous_case);
      auto &prev_value = std::get<1>(previous_case);
      auto &prev_mask = std::get<2>(previous_case);

      if (prev_prefix_length > prefix_length) {
        continue;
      }
      // The current prefix is at least as long, check that there aren't any
      // keys that can match both cases.
      bool overlap = true;
      for (size_t i = 0; i < mask.size(); ++i) {
        // A key matches both cases if the two values agree on the bits
        // corresponding to the intersection of the masks.
        if ((mask[i] && prev_mask[i]) && (value[i] != prev_value[i])) {
          // There is a bit in the intersection of the masks that the values
          // differ, so these cases do not overlap.
          overlap = false;
          break;
        }
      }
      if (overlap) {
        ReportWarningAt(
            select_case->getSourceInfo(),
            "This select case overlaps with a previous select case, so the "
            "behavior of the P4 parser (first case matches) and the output "
            "TCAM program (longest prefix matches) may differ.");
      }
    }
    previous_cases.push_back({prefix_length, value, mask});
  }
  return false;
}

bool IRBuilder::ExtractImpl(const IR::MethodCallStatement *method_call) {
  auto state = findContext<IR::ParserState>();
  if (!state) {
    // This method call is outside a parser state, so we cannot translate it
    return false;
  }
  auto &code_gen_info = code_gen_info_.at(state->name.toString().c_str());

  // The TCAM entry for the in-state for this parser state. We generate
  // instructions for the in-state out of statements.
  auto &tcam_in = tcam_program_.FindOrInsertTCAMEntry(
      InState(state->name.toString().c_str()), {}, {});

  auto header_arg = method_call->methodCall->arguments->at(0);
  if (auto header_field_expr =
          this->ResolveHeaderExpression(header_arg->expression)) {
    if (header_field_expr->size() != 1) {
      ReportErrorAt(
          header_arg->getSourceInfo(),
          "The argument to extract is a nested field access, we do not "
          "support extracting nested headers.");
      return false;
    }
    auto header = header_field_expr->at(0);
    if (!tcam_program_.header_instantiations_.contains(header.name.c_str())) {
      ReportErrorAt(header_arg->getSourceInfo(), "The extracted header ",
                    header.toString(), " is not declared.");
      return false;
    }

    const size_t header_size = tcam_program_.HeaderSize(header.name.c_str());
    const auto begin_offset = code_gen_info.current_offset;
    // Mark the header as available
    code_gen_info.header_offsets.emplace(header.name, begin_offset);
    // Extract header fields
    auto field_offset = begin_offset;
    const auto header_fields = tcam_program_.HeaderFields(header.name.c_str());
    if (!header_fields) {
      ReportErrorAt(header.getSourceInfo(), "The type of the header ",
                    header.name,
                    " is unknown, skipping extracting its fields.");
      return false;
    }
    const auto prefix = std::string(header.name) + ".";
    for (const auto &field_info : *header_fields) {
      auto &field = field_info.first;
      auto &size = field_info.second;
      tcam_in.instructions.push_back(std::make_shared<StoreHeaderField>(
          util::Range::Create(field_offset, field_offset + size).value(),
          prefix + field));
      field_offset += size;
    }

    // Move the cursor
    code_gen_info.current_offset += header_size;
    tcam_in.instructions.push_back(std::make_shared<Move>(header_size));
  } else {
    ReportErrorAt(header_arg->getSourceInfo(),
                  "The argument to extract is not member access to the header "
                  "argument (expected an expression like `",
                  header_param_->toString(), ".foo`).");
    return false;
  }
  return true;
}

}  // namespace backends::tc
