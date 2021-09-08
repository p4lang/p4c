#include "backends/tc/dead_state_elimination.h"

#include <cstdint>
#include <memory>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/memory/memory.h"
#include "backends/tc/pass.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/test_util.h"
#include "backends/tc/util.h"
#include "gtest/gtest.h"

namespace backends::tc {
namespace {

class DeadStateEliminationTest : public testing::Test {};

TEST_F(DeadStateEliminationTest, SimpleStateGraph) {
  // In the following program, T and V are not reachable from start, but S and U
  // are.
  auto tcam_program = TCAMProgramWithGivenTransitions({
      {State(kStartState), {"S"}},
      {"S", {"U"}},
      {"T", {"V", State(kRejectState)}},
      {"U", {State(kAcceptState)}},
      {"V", {State(kRejectState)}},
  });

  PassManager pass_manager;
  pass_manager.AddPass(absl::make_unique<DeadStateElimination>(pass_manager));
  auto processed_program = pass_manager.RunPasses(tcam_program);
  EXPECT_TRUE(processed_program) << "DeadStateElimination pass failed.";
  auto& tcam_table = processed_program->tcam_table_;
  EXPECT_TRUE(tcam_table.contains("start"));
  EXPECT_TRUE(tcam_table.contains("S"));
  EXPECT_TRUE(tcam_table.contains("U"));
  EXPECT_FALSE(tcam_table.contains("T"));
  EXPECT_FALSE(tcam_table.contains("V"));
}

}  // namespace
}  // namespace backends::tc
