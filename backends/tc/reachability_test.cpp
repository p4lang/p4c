#include "backends/tc/reachability.h"

#include <cstdint>
#include <memory>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "backends/tc/pass.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/test_util.h"
#include "backends/tc/util.h"
#include "gtest/gtest.h"

namespace backends::tc {
namespace {

class ReachabilityTest : public testing::Test {};

TEST_F(ReachabilityTest, DirectReachability) {
  // All reachable states are directly reachable or the state itself in the
  // following program
  auto tcam_program = TCAMProgramWithGivenTransitions({
      {State(kStartState), {"S", "U", "accept"}},
      {"S", {"accept", "U"}},
      {"T", {"reject"}},
      {"U", {"accept"}},
  });

  PassManager pass_manager;
  pass_manager.RunPasses(tcam_program);
  auto reachability_result = pass_manager.GetAnalysisResult<Reachability>();
  EXPECT_TRUE(reachability_result) << "Reachability analysis failed.";
  auto& transitive_closure = reachability_result->transitive_closure;
  EXPECT_EQ(
      transitive_closure.at(kStartState),
      (absl::flat_hash_set<State>{State(kStartState), "S", "U", "accept"}));
  EXPECT_EQ(transitive_closure.at("S"),
            (absl::flat_hash_set<State>{"S", "U", "accept"}));
  EXPECT_EQ(transitive_closure.at("T"),
            (absl::flat_hash_set<State>{"T", "reject"}));
  EXPECT_EQ(transitive_closure.at("U"),
            (absl::flat_hash_set<State>{"U", "accept"}));
}

TEST_F(ReachabilityTest, IndirectReachability) {
  auto tcam_program = TCAMProgramWithGivenTransitions({
      {State(kStartState), {"S"}},
      {"S", {"U"}},
      {"T", {"V", "reject"}},
      {"U", {"accept"}},
      {"V", {"reject"}},
  });

  PassManager pass_manager;
  pass_manager.RunPasses(tcam_program);
  auto reachability_result = pass_manager.GetAnalysisResult<Reachability>();
  EXPECT_TRUE(reachability_result) << "Reachability analysis failed.";
  auto& transitive_closure = reachability_result->transitive_closure;
  EXPECT_EQ(
      transitive_closure.at(kStartState),
      (absl::flat_hash_set<State>{State(kStartState), "S", "U", "accept"}));
  EXPECT_EQ(transitive_closure.at("S"),
            (absl::flat_hash_set<State>{"S", "U", "accept"}));
  EXPECT_EQ(transitive_closure.at("T"),
            (absl::flat_hash_set<State>{"T", "V", "reject"}));
  EXPECT_EQ(transitive_closure.at("U"),
            (absl::flat_hash_set<State>{"U", "accept"}));
  EXPECT_EQ(transitive_closure.at("V"),
            (absl::flat_hash_set<State>{"V", "reject"}));
}

}  // namespace
}  // namespace backends::tc
