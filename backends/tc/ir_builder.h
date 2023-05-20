#ifndef BACKENDS_TC_IR_BUILDER_H_
#define BACKENDS_TC_IR_BUILDER_H_

#include <optional>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/util.h"
#include "frontends/p4/frontend.h"
#include "ir/ir.h"

namespace backends::tc {

class IRBuilderError : public std::exception {};

// Ephemeral information used for generating instructions for a state.
struct CodeGenInfo {
  // How much the parser has advanced since the beginning of this state.
  size_t current_offset = 0;
  // Which headers are extracted and available for transitioning from the
  // current state, and their starting offsets
  absl::flat_hash_map<std::string, size_t> header_offsets;
  // Mapping from variables to the offset into the packet buffer they are
  // assigned to. The keys of the map are the declarations for each local
  // variable that is available. This map is populated when processing calls to
  // packet.lookahead
  absl::flat_hash_map<const IR::IDeclaration*, util::Range> available_locals;
};

// The pass that builds our IR (as opposed to P4C's IR). This is technically the
// "frontend" of our first pass, so it is not part of the pass manager
// infrastructure.
//
// It is built as a visitor over P4's IR
class IRBuilder final : public Inspector {
 public:
  IRBuilder(P4::ReferenceMap& ref_map, P4::TypeMap& type_map)
      : ref_map_(ref_map), type_map_(type_map) {
    setName("TcIRBuilder");
  }

  // visitor methods to traverse the parse graph

  // Visit the parser and generate header types and declarations for headers
  // used in the parser.
  bool preorder(const IR::P4Parser* parser) override;
  // Visit the state to create the CodeGenInfo for that state.
  bool preorder(const IR::ParserState* state) override;
  // Visit the statements inside a state to generate the tc productions for the
  // "in-state" (the part of the state that extracts headers and sets the TCAM
  // lookup key, see README for in-states and out-states).  This visitor does
  // not generate the SetKey and NextState instructions for the in-state. Those
  // instructions are generated when processing the transition statements in
  // other visitors.
  bool preorder(const IR::Statement* statement) override;
  // Visit the path expression inside a state's transition statement to generate
  // the instructions for that state transition. This visitor generates the
  // instructions to connect the "in-state" to the "out-state", and the
  // "out-state" to the next state's "in-state".
  bool preorder(const IR::PathExpression* expression) override;
  // Visit the select expression inside a state's transition statement to
  // generate the `SetKey` and `NextState` instructions for that state
  // transition. This visitor generates the instructions to connect the
  // "in-state" to the "out-state", and the "out-state" to the next state's
  // "in-state". (see README for in-states and out-states)
  bool preorder(const IR::SelectExpression* expression) override;

  // Extract the fields of the given header. The prefix indicates where in the
  // header field hierarchy we are. Returns a map from field path to bit-width
  // of primitive-typed fields.
  //
  // For example, when we are extracting the top-level headers, the prefix is ""
  //
  // When we are extracting fields of a field named "foo", then the prefix is
  // "foo."
  std::vector<std::pair<std::string, size_t>> ExtractHeaderFields(
      const std::string& prefix, const IR::Type_StructLike* header);

  const TCAMProgram& tcam_program() { return this->tcam_program_; }

  bool has_errors() const { return this->has_errors_; }

 private:
  // Whether the IR builder has encountered any errors. This field is marked
  // mutable so we can report errors inside methods marked const.
  mutable bool has_errors_ = false;

  // Mapping from paths to declarations in the P4 program to resolve any
  // variables encountered to the correct declaration.
  const P4::ReferenceMap& ref_map_;
  // Mapping of P4 IR nodes to their types from the P4C front-end
  const P4::TypeMap& type_map_;
  // The generated TCAM program
  TCAMProgram tcam_program_;
  // The header parameter used by the P4 parser, we use this to find the
  // extracted headers.
  absl::optional<IR::ID> header_param_;

  // Methods supported by this backend, mapped to their implementations.
  const absl::flat_hash_map<
      absl::string_view,
      std::function<bool(IRBuilder&, const IR::MethodCallStatement*)>>
      kSupportedMethods = {
          {"extract", [](IRBuilder& ir_builder,
                         const IR::MethodCallStatement* method_call) {
             return ir_builder.ExtractImpl(method_call);
           }}};

  // Name of packet.lookahead, this method is kept separately from other methods
  // we support because it occurs in a different context (right-hand side of
  // assignments) from other methods (other methods appear as top-level
  // statements). So, its functionality requires additional context that is not
  // supplied to the method implementations.
  const IR::ID kLookahead = "lookahead";

  // Code generation information per state, this is used for keeping track of
  // available fields and how many bits the parser has advanced in the middle of
  // execution.
  absl::flat_hash_map<std::string, CodeGenInfo> code_gen_info_;

  // Resolve an expression that should accesses a field of the header parameter
  // in the parser. It returns a vector representing the sequence of field
  // accesses. For example, on the input `hdr.foo.bar` it returns ["foo", "bar"]
  // if hdr is the header parameter of the parser. It returns nullopt if the
  // expression is not a field access, or if it is accessing the fields of a
  // variable other than header_param_.
  absl::optional<std::vector<IR::ID>> ResolveHeaderExpression(
      const IR::Expression* expr) const;

  // Get ranges from the packet buffer relative to the cursor at the beginning
  // of current state. This is used for concatenating values and emitting
  // set-key instructions.
  std::vector<util::Range> GetRangesRelativeToCursor(
      const std::string& current_state,
      const IR::ListExpression* select_guard) const;

  // Collect the values and masks in a keyset expression, this returns a vector
  // where each element corresponds to the relevant tuple element in the select
  // expression.
  absl::optional<std::vector<std::pair<std::vector<bool>, std::vector<bool>>>>
  CollectValuesAndMasks(const IR::Expression* keyset_expression) const;

  // Report a compiler error associated with a location. This is a type-safe
  // wrapper around `::error` from P4C that also handles some boilerplate for
  // error reporting and keeping track of compiler errors.
  template <typename... T>
  void ReportErrorAt(Util::SourceInfo error_location,
                     T... message_contents) const {
    std::ostringstream message;
    message << "At " << error_location.toPositionString() << ":\n"
            << error_location.toSourceFragment() << "\n";
    util::FoldLeft<util::WriteToStream, std::ostream&>(message,
                                                       message_contents...);
    ::error("%s", message.str());
    this->has_errors_ = true;
  }

  // Report a compiler warning associated with a location. This is a type-safe
  // wrapper around `::warning` from P4C.
  template <typename... T>
  void ReportWarningAt(Util::SourceInfo error_location,
                       T... message_contents) const {
    std::ostringstream message;
    message << "At " << error_location.toPositionString() << ":\n"
            << error_location.toSourceFragment() << "\n";
    util::FoldLeft<util::WriteToStream, std::ostream&>(message,
                                                       message_contents...);
    ::warning("%s", message.str());
  }

  // Report a compiler error associated with a location and bail out with an
  // exception. We throw an exception because P4C already uses exceptions, and
  // because of how it implements the visitor pattern, there is no other way of
  // aborting visiting the rest of the IR.
  template <typename... T>
  void UnrecoverableErrorAt(Util::SourceInfo error_location,
                            T... message_contents) const {
    ReportErrorAt(error_location, message_contents...);
    throw IRBuilderError{};
  }

 private:
  // Implementation of supported methods. All implementations return whether
  // they successfully translate the method call.

  // Implementation of packet.extract()
  bool ExtractImpl(const IR::MethodCallStatement*);
  // End implementation of supported methods
};

}  // namespace backends::tc

#endif  // BACKENDS_TC_IR_BUILDER_H_
