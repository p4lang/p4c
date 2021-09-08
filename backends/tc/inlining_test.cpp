#include "backends/tc/inlining.h"

#include <memory>

#include "absl/memory/memory.h"
#include "backends/tc/pass.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/test_util.h"
#include "backends/tc/util.h"
#include "gtest/gtest.h"

namespace backends::tc {
namespace {

class InliningTest : public testing::Test {
 protected:
  void SetUp() override {
    // Reset the pass manager
    pass_manager_ = PassManager();
    pass_manager_.AddPass(absl::make_unique<Inlining>(pass_manager_));
  }

  PassManager pass_manager_;
};

TEST_F(InliningTest, InlineSimpleTransition) {
  // Test inlining simple TCAM entries that have NextState as the only
  // instruction.
  //
  // The start_out state has two entries that can be inlined, one of them
  // transitions to a special state (accept).
  auto tcam_program = TCAMProgramWithTestHeaders(
      TCAMTable{{"start",
                 {
                     TCAMEntry{.value = {},
                               .mask = {},
                               .instructions =
                                   {
                                       std::make_shared<NextState>("start_out"),
                                       std::make_shared<SetKey>(
                                           util::Range::Create(0, 16).value()),
                                       std::make_shared<Move>(16),
                                   }},
                 }},
                {"start_out",
                 {TCAMEntry{
                      .value = util::UInt64ToBitString(0, 16),
                      .mask = util::UInt64ToBitString(0, 16),
                      .instructions =
                          {
                              std::make_shared<NextState>("accept"),
                          },
                  },
                  TCAMEntry{
                      .value = util::UInt64ToBitString(0x0800, 16),
                      .mask = util::UInt64ToBitString(0xFFFF, 16),
                      .instructions =
                          {
                              std::make_shared<NextState>("S"),
                          },
                  }}},
                {"S",
                 {TCAMEntry{
                     .value = {},
                     .mask = {},
                     .instructions =
                         {
                             std::make_shared<Move>(16),
                             std::make_shared<StoreHeaderField>(
                                 util::Range::Create(0, 8).value(), "foo.bar"),
                             std::make_shared<StoreHeaderField>(
                                 util::Range::Create(8, 16).value(), "foo.baz"),
                             std::make_shared<NextState>("accept"),
                         },
                 }}}});

  auto new_program = pass_manager_.RunPasses(tcam_program);
  EXPECT_TRUE(new_program) << "Inlining pass failed";
  // Validate that "start" and "S" have the same entries as before
  EXPECT_EQ(new_program->tcam_table_.at("start"),
            tcam_program.tcam_table_.at("start"));
  EXPECT_EQ(new_program->tcam_table_.at("S"), tcam_program.tcam_table_.at("S"));
  // Check inlining non-special state "S"
  auto entry_calling_S = new_program->FindTCAMEntry(
      "start_out", util::UInt64ToBitString(0x0800, 16),
      util::UInt64ToBitString(0xFFFF, 16));
  EXPECT_EQ(*entry_calling_S,
            (TCAMEntry{.value = util::UInt64ToBitString(0x0800, 16),
                       .mask = util::UInt64ToBitString(0xFFFF, 16),
                       .instructions = {
                           std::make_shared<Move>(16),
                           std::make_shared<StoreHeaderField>(
                               util::Range::Create(0, 8).value(), "foo.bar"),
                           std::make_shared<StoreHeaderField>(
                               util::Range::Create(8, 16).value(), "foo.baz"),
                           std::make_shared<NextState>("accept"),
                       }}));
  // Check inlining special state "accept"
  auto entry_calling_accept =
      new_program->FindTCAMEntry("start_out", util::UInt64ToBitString(0, 16),
                                 util::UInt64ToBitString(0, 16));
  EXPECT_EQ(*entry_calling_accept,
            (TCAMEntry{.value = util::UInt64ToBitString(0, 16),
                       .mask = util::UInt64ToBitString(0, 16),
                       .instructions = {
                           std::make_shared<NextState>("accept"),
                       }}));
}

TEST_F(InliningTest, InlineTransitively) {
  // Test inlining transitively.
  //
  // - The call graph is: start -> S -> T -> accept
  // - S and T just transition to the next state
  // - After inlining; start, S, and T should have the same instructions as the
  // original instructions of T.
  auto tcam_program = TCAMProgramWithTestHeaders(
      TCAMTable{{"start",
                 {
                     TCAMEntry{.value = {},
                               .mask = {},
                               .instructions =
                                   {
                                       std::make_shared<NextState>("S"),
                                   }},
                 }},
                {"S",
                 {
                     TCAMEntry{.value = {},
                               .mask = {},
                               .instructions =
                                   {
                                       std::make_shared<NextState>("T"),
                                   }},
                 }},
                {"T",
                 {TCAMEntry{
                     .value = {},
                     .mask = {},
                     .instructions =
                         {
                             std::make_shared<Move>(16),
                             std::make_shared<StoreHeaderField>(
                                 util::Range::Create(0, 8).value(), "foo.bar"),
                             std::make_shared<StoreHeaderField>(
                                 util::Range::Create(8, 16).value(), "foo.baz"),
                             std::make_shared<NextState>("accept"),
                         },
                 }}}});

  auto new_program = pass_manager_.RunPasses(tcam_program);
  EXPECT_TRUE(new_program) << "Inlining pass failed";
  // We expect all states to have the same instructions as T's original
  // instructions
  const auto& expected_instructions =
      tcam_program.FindTCAMEntry("T", {}, {})->instructions;
  EXPECT_EQ(*(new_program->FindTCAMEntry("start", {}, {})),
            (TCAMEntry{{}, {}, expected_instructions}));
  EXPECT_EQ(*(new_program->FindTCAMEntry("S", {}, {})),
            (TCAMEntry{{}, {}, expected_instructions}));
  EXPECT_EQ(*(new_program->FindTCAMEntry("T", {}, {})),
            (TCAMEntry{{}, {}, expected_instructions}));
}

TEST_F(InliningTest, InlineWithMove) {
  // Test inlining simple TCAM entries that have instructions besides NextState,
  // including Move.
  //
  // start moves the cursor then goes to S, and S includes instructions that
  // operate relative to the cursor.
  auto tcam_program = TCAMProgramWithTestHeaders(TCAMTable{
      {"start",
       {
           TCAMEntry{.value = {},
                     .mask = {},
                     .instructions =
                         {
                             std::make_shared<NextState>("S"),
                             std::make_shared<SetKey>(
                                 util::Range::Create(0, 16).value()),
                             std::make_shared<Move>(16),
                         }},
       }},
      {"S",
       {TCAMEntry{
           .value = {},
           .mask = {},
           .instructions =
               {
                   std::make_shared<Move>(16),
                   std::make_shared<StoreHeaderField>(
                       util::Range::Create(0, 8).value(), "foo.bar"),
                   std::make_shared<StoreHeaderField>(
                       util::Range::Create(8, 16).value(), "foo.baz"),
                   std::make_shared<SetKey>(util::Range::Create(8, 16).value()),
                   std::make_shared<NextState>("accept"),
               },
       }}}});

  auto new_program = pass_manager_.RunPasses(tcam_program);
  EXPECT_TRUE(new_program) << "Inlining pass failed";
  // Check inlining "S" with appropriate shifts in offset
  auto entry_calling_S = new_program->FindTCAMEntry("start", {}, {});
  auto expected_entry = TCAMEntry{
      {},
      {},
      {
          std::make_shared<SetKey>(util::Range::Create(0, 16).value()),
          std::make_shared<Move>(16),
          std::make_shared<Move>(16),
          std::make_shared<StoreHeaderField>(
              util::Range::Create(16, 24).value(), "foo.bar"),
          std::make_shared<StoreHeaderField>(
              util::Range::Create(24, 32).value(), "foo.baz"),
          std::make_shared<SetKey>(util::Range::Create(24, 32).value()),
          std::make_shared<NextState>("accept"),
      }};
  EXPECT_EQ(*entry_calling_S, expected_entry);
}

}  // namespace
}  // namespace backends::tc
