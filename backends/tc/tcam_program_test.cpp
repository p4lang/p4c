#include "backends/tc/tcam_program.h"

#include "backends/tc/test_util.h"

#include "gtest/gtest.h"

namespace backends::tc {
namespace {

class TcamProgramTest : public testing::Test {
 protected:
  void SetUp() override {
    tcam_program_ = TCAMProgramWithTestHeaders(TCAMTable{
        {"start",
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
                      std::make_shared<SetKey>(
                          util::Range::Create(0, 16).value()),
                      std::make_shared<NextState>("S"),
                  },
          }}},
        {"S",
         {TCAMEntry{
             .value = util::UInt64ToBitString(0x1234, 16),
             .mask = util::UInt64ToBitString(0xFFFF, 16),
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
  }

  TCAMProgram tcam_program_;
};

TEST_F(TcamProgramTest, FindMatchingEntryFailures) {
  auto entry = tcam_program_.FindMatchingEntry("Foo", Value{});
  EXPECT_FALSE(entry) << " did not expect to match an entry for a non-existing "
                         "state, but matched "
                      << *entry;

  entry = tcam_program_.FindMatchingEntry("S", util::UInt64ToBitString(0x5678));
  EXPECT_FALSE(entry)
      << " did not expect to match an entry for the given key, but matched "
      << *entry;
}

TEST_F(TcamProgramTest, FindMatchingEntryEmptyMask) {
  auto entry = tcam_program_.FindMatchingEntry("start", Value{});
  ASSERT_TRUE(entry);
  EXPECT_EQ(*entry, (TCAMEntry{.value = {},
                               .mask = {},
                               .instructions = {
                                   std::make_shared<NextState>("start_out"),
                                   std::make_shared<SetKey>(
                                       util::Range::Create(0, 16).value()),
                                   std::make_shared<Move>(16),
                               }}));
}

TEST_F(TcamProgramTest, FindMatchingEntryLongestPrefixMatch) {
  // Multiple entries match, should pick the longest one
  auto entry = tcam_program_.FindMatchingEntry(
      "start_out", util::UInt64ToBitString(0x0800, 16));
  ASSERT_TRUE(entry);
  EXPECT_EQ(
      *entry,
      (TCAMEntry{
          .value = util::UInt64ToBitString(0x0800, 16),
          .mask = util::UInt64ToBitString(0xFFFF, 16),
          .instructions =
              {
                  std::make_shared<SetKey>(util::Range::Create(0, 16).value()),
                  std::make_shared<NextState>("S"),
              },
      }));

  // The longest entry does not match but the other one matches
  entry = tcam_program_.FindMatchingEntry("start_out",
                                          util::UInt64ToBitString(0x1234, 16));
  ASSERT_TRUE(entry);
  EXPECT_EQ(*entry, (TCAMEntry{
                        .value = util::UInt64ToBitString(0, 16),
                        .mask = util::UInt64ToBitString(0, 16),
                        .instructions =
                            {
                                std::make_shared<NextState>("accept"),
                            },
                    }));
}

}  // namespace
}  // namespace backends::tc
