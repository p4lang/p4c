#include <algorithm>
#include <fstream>
#include <memory>

#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "backends/tc/instruction.h"
#include "backends/tc/ir_builder.h"
#include "backends/tc/p4c_interface.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/util.h"
#include "gtest/gtest.h"

namespace backends::tc {
namespace {

static constexpr char kTupleTestProgram[] = "backends/tc/testdata/tuple.json";

class TupleTest : public testing::Test {
 protected:
  void SetUp() override {
    // Arguments to pass to the compiler.  p4c expects a char * array, rather
    // than a const char * array. So, we convert everything to string
    // implicitly, then take a reference to the first character. In the future
    // (when using C++17) we can switch to data() to create a second array of
    // pointers to data.
    std::string p4_program = kTupleTestProgram;

    std::vector<std::string> args{"p4c-tc", "--fromJSON", p4_program};
    std::vector<char *> argv;
    for (auto &arg : args) {
      argv.push_back(&arg[0]);
    }

    // Set up P4C's infrastructure
    setup_gc_logging();
    setup_signals();

    AutoCompileContext compileContext(new P4TcContext);
    auto &options = P4TcContext::get().options();

    // Support P4-16 only
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;

    // Parse compiler arguments
    options.process(argv.size(), argv.data());
    ASSERT_EQ(::errorCount(), 0);

    auto debug_hook = options.getDebugHook();

    // Load the IR
    std::ifstream json(options.file);
    ASSERT_TRUE(json) << "Could not open the JSON file containing P4 IR: "
                      << options.file;
    JSONLoader loader(json);
    const IR::Node *node = nullptr;
    loader >> node;
    const IR::P4Program *program = node->to<IR::P4Program>();
    ASSERT_TRUE(program) << options.file << "is not a P4 IR in JSON format.";

    // Run the P4C mid-end
    MidEnd mid_end(options);
    mid_end.addDebugHook(debug_hook);
    const IR::ToplevelBlock *toplevel = nullptr;
    toplevel = mid_end.process(program);

    ASSERT_EQ(::errorCount(), 0);

    backends::tc::IRBuilder ir_builder(mid_end.ref_map_, mid_end.type_map_);

    program->apply(ir_builder);
    ASSERT_FALSE(ir_builder.has_errors());

    delete program;

    tcam_program_ = ir_builder.tcam_program();
  }

  void TearDown() override {}

  absl::optional<TCAMProgram> tcam_program_;
};

// Helper function to find the last SetKey instruction in an entry
const SetKey *FindLastSetKeyInstruction(const TCAMEntry &tcam_entry) {
  const auto set_key = std::find_if(
      tcam_entry.instructions.rbegin(), tcam_entry.instructions.rend(),
      [](const std::shared_ptr<const Instruction> &instruction) {
        return bool(dynamic_cast<const SetKey *>(instruction.get()));
      });
  if (set_key == tcam_entry.instructions.rend()) {
    return nullptr;
  }
  return dynamic_cast<const SetKey *>(set_key->get());
}

TEST_F(TupleTest, TupleWithNestedDefaults) {
  ASSERT_TRUE(tcam_program_)
      << "The IR builder has not generated the TCAM program";
  // Check if the set-key instruction generated for start state has the correct
  // offsets.
  const auto start_entry = tcam_program_->FindTCAMEntry("start", {}, {});
  ASSERT_NE(start_entry, nullptr)
      << "There is no entry for start, value=0, mask=0";
  const auto set_key = FindLastSetKeyInstruction(*start_entry);
  ASSERT_NE(set_key, nullptr) << "Could not find the set-key instruction";
  // The key should use all of ethernet packet
  ASSERT_EQ(
      set_key->range(),
      util::Range::Create(0, tcam_program_->HeaderSize("ethernet")).value())
      << "The range in the set-key instruction does not match the expected "
         "one.";
  // Check the productions for each select case
  const auto &start_out_entries = tcam_program_->tcam_table_.at("start_out");
  // Create a set so that we compare the TCAM entries without checking the
  // ordering
  absl::flat_hash_set<TCAMEntry> actual_entries;
  for (const auto &entry : start_out_entries) {
    actual_entries.insert(entry);
  }
  const absl::flat_hash_set<TCAMEntry> expected_entries{
      TCAMEntry{util::HexStringToBitString(112, "FFFFFFFFFFFF0000000000000007"),
                util::HexStringToBitString(112, "FFFFFFFFFFFF000000000000F00F"),
                {std::make_shared<NextState>("reject")}},
      TCAMEntry{util::HexStringToBitString(112, "FFFFFFFFFFFF0000000000000000"),
                util::HexStringToBitString(112, "FFFFFFFFFFFF0000000000000000"),
                {std::make_shared<NextState>("state2_in")}},
      TCAMEntry{util::HexStringToBitString(112, "0"),
                util::HexStringToBitString(112, "0"),
                {std::make_shared<NextState>("accept")}},
  };
  ASSERT_EQ(actual_entries, expected_entries)
      << "The expected and actual TCAM entries for start_out do not match.";
}

}  // namespace
}  // namespace backends::tc
