/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <fstream>
#include <ostream>
#include <string>

#include "bf-p4c/device.h"
#include "bf-p4c/mau/mau_power.h"
#include "bf-p4c/mau/memories.h"
#include "lib/hex.h"

namespace MauPower {

std::ostream &operator<<(std::ostream &out, mau_dep_t dep) {
  if (dep == DEP_MATCH) {
    out << "match";
  } else if (dep == DEP_ACTION) {
    out << "action";
  } else {
    out << "concurrent";
  }
  return out;
}

std::string toString(mau_dep_t dep) {
  std::stringstream tmp;
  tmp << dep;
  return tmp.str();
}

std::string float2str(double d) {
  std::ostringstream s;
  s << std::fixed;
  s << std::setprecision(2);
  s << d;
  return s.str();
}

int MauFeatures::get_max_selector_words(gress_t gress, int stage) const {
  return max_selector_words_[gress][stage];
}

bool MauFeatures::stage_has_feature(gress_t gress, int stage, stage_feature_t feature) const {
  switch (feature) {
  case HAS_EXACT: return has_exact_[gress][stage];
  case HAS_TCAM: return has_tcam_[gress][stage];
  case HAS_STATS: return has_stats_[gress][stage];
  case HAS_SEL: return has_selector_[gress][stage];
  case HAS_LPF_OR_WRED: return has_meter_lpf_or_wred_[gress][stage];
  case HAS_STFUL: return has_stateful_[gress][stage];
  case WIDE_SEL: return max_selector_words_[gress][stage] > 0;
  default: BUG("Unkown feature %s", feature); }
  return false;
}

std::ostream& MauFeatures::emit_dep_asm(std::ostream& out, gress_t g, int stage) const {
  // The ghost thread uses the same dependency as ingress, so if tables in
  // the ghost thread induce a dependency, it has already been merged into ingress.
  // Skip stage dependency for stage 0 as it has no previous stage
  if ((g != GHOST) && (stage > 0)) {
    mau_dep_t dep = get_dependency_for_gress_stage(g, stage);
    out << "  dependency: " << dep << std::endl;
  }
  return out;
}
bool MauFeatures::requires_dep_asm(gress_t g, int stage) const {
  if ((g != GHOST) && (stage > 0)) {
    mau_dep_t dep = get_dependency_for_gress_stage(g, stage);
    if (Device::currentDevice() == Device::TOFINO)
        return dep != DEP_CONCURRENT;
    else
        return dep != DEP_ACTION; }
  return false;
}

mau_dep_t MauFeatures::get_dependency_for_gress_stage(gress_t g, int stage) const {
  return ::get(stage_dep_to_previous_[g], stage, DEP_MATCH);
}
void MauFeatures::set_dependency_for_gress_stage(gress_t g, int stage, mau_dep_t dep) {
  stage_dep_to_previous_[g][stage] = dep;
}

bool MauFeatures::try_convert_to_match_dep() {
  for (gress_t gress : Device::allGresses()) {
    for (int s=0; s < Device::numStages(); ++s) {
      mau_dep_t dep = get_dependency_for_gress_stage(gress, s);
      if (dep != DEP_MATCH) {
        LOG4("Convert " << gress << " stage " << s << " to match dependent.");
        set_dependency_for_gress_stage(gress, s, DEP_MATCH);
        // Ingress and Ghost have to have the same dependency types.
        if (gress == INGRESS) {
          set_dependency_for_gress_stage(GHOST, s, DEP_MATCH);
        } else if (gress == GHOST) {
          set_dependency_for_gress_stage(INGRESS, s, DEP_MATCH);
        }
        return true;
      }
    }
  }
  return false;
}

// There is no concurrent on Tofino2 and beyond.
void MauFeatures::update_deps_for_device() {
  if (Device::currentDevice() != Device::TOFINO) {
    for (gress_t gress : Device::allGresses()) {
      for (int stage=0; stage < Device::numStages(); ++stage) {
        if (get_dependency_for_gress_stage(gress, stage) == DEP_CONCURRENT)
          set_dependency_for_gress_stage(gress, stage, DEP_ACTION);
      }
    }

    // Ingress and Ghost have to have the same timing, which requires them
    // to have the same dependency type.
    for (int stage=0; stage < Device::numStages(); ++stage) {
      mau_dep_t i_dep = get_dependency_for_gress_stage(INGRESS, stage);
      mau_dep_t g_dep = get_dependency_for_gress_stage(GHOST, stage);
      if (i_dep != g_dep) {
        set_dependency_for_gress_stage(INGRESS, stage, DEP_MATCH);
        set_dependency_for_gress_stage(GHOST, stage, DEP_MATCH); } }
  }
}

int MauFeatures::compute_pipe_latency(gress_t gress) const {
  auto& spec = Device::mauPowerSpec();
  int lat = spec.get_mau_corner_turn_delay();
  for (int stage=0; stage < Device::numStages(); ++stage) {
    mau_dep_t dep = get_dependency_for_gress_stage(gress, stage);
    if (dep == DEP_CONCURRENT) {
      lat += spec.get_concurrent_latency_contribution();
    } else if (dep == DEP_ACTION) {
      lat += spec.get_action_latency_contribution();
    } else {
      lat += compute_stage_latency(gress, stage);
    }
  }
  return lat;
}

int MauFeatures::compute_pred_cycle(gress_t gress, int stage) const {
  auto& spec = Device::mauPowerSpec();
  int pred_cycle = spec.get_base_predication_delay();
  if (stage_has_feature(gress, stage, HAS_TCAM) ||
      stage_has_chained_feature(gress, stage, HAS_TCAM)) {
    pred_cycle += spec.get_tcam_delay();
  } else if (stage_has_feature(gress, stage, HAS_SEL) &&
             stage_has_feature(gress, stage, WIDE_SEL)) {
    // Note: chaining wide selector case is covered by
    // stage_has_chained_feature(gress, stage, HAS_TCAM)
    pred_cycle += spec.get_tcam_delay();
  }
  return pred_cycle;
}

int MauFeatures::compute_stage_latency(gress_t gress, int stage) const {
  auto& spec = Device::mauPowerSpec();
  int lat = spec.get_base_delay();
  if (stage_has_feature(gress, stage, HAS_TCAM) ||
      stage_has_feature(gress, stage, WIDE_SEL)) {
    lat += spec.get_tcam_delay();
  } else if (stage_has_chained_feature(gress, stage, HAS_TCAM)) {
    lat += spec.get_tcam_delay();
  }
  if (stage_has_feature(gress, stage, HAS_SEL) ||
      stage_has_chained_feature(gress, stage, HAS_SEL)) {
    lat += spec.get_selector_delay();
  } else {
    if (stage_has_feature(gress, stage, HAS_LPF_OR_WRED) ||
        stage_has_chained_feature(gress, stage, HAS_LPF_OR_WRED)) {
      lat += spec.get_meter_lpf_delay();
    } else if (stage_has_feature(gress, stage, HAS_STFUL) ||
               stage_has_chained_feature(gress, stage, HAS_STFUL)) {
      lat += spec.get_stateful_delay();
    }
  }
  return lat;
}

bool MauFeatures::stage_has_chained_feature(gress_t gress, int stage,
                                            stage_feature_t feature) const {
  // Look earlier in the chain.
  mau_dep_t dep_to_prev = get_dependency_for_gress_stage(gress, stage);
  int look_stage = stage;
  while ((look_stage % Device::numStages()) > 0 &&
    dep_to_prev != DEP_MATCH) {
    --look_stage;
    dep_to_prev = get_dependency_for_gress_stage(gress, look_stage);
    if (stage_has_feature(gress, look_stage, feature))
      return true;
    // For latency reasons, say has TCAM if have wide selector.
    if (feature == HAS_TCAM &&
      stage_has_feature(gress, look_stage, HAS_SEL) &&
      stage_has_feature(gress, look_stage, WIDE_SEL))
      return true;
  }
  // Look later in the chain if are stages to look at.
  if ((stage % Device::numStages()) == (Device::numStages() - 1))
    return false;
  look_stage = stage + 1;
  while (true) {
    dep_to_prev = get_dependency_for_gress_stage(gress, look_stage);
    if (dep_to_prev == DEP_MATCH)
      break;
    if (stage_has_feature(gress, look_stage, feature))
      return true;
    // For latency reasons, say has TCAM if have wide selector.
    if (feature == HAS_TCAM &&
      stage_has_feature(gress, look_stage, HAS_SEL) &&
      stage_has_feature(gress, look_stage, WIDE_SEL))
      return true;
    // If we're already at the last stage, terminate loop.
    if ((look_stage % Device::numStages()) == (Device::numStages() - 1)) {
      break; }
    ++look_stage;
  }
  return false;
}


bool MauFeatures::are_there_more_tables(gress_t gress, int start_stage) const {
  for (int s = start_stage; s < Device::numStages(); ++s) {
    if (stage_to_tables_[gress][s].size() != 0)
      return true;
  }
  return false;
}

void MauFeatures::print_features(std::ostream& out, gress_t gress) const {
  std::stringstream heading;
  std::stringstream sep;
  for (int i = 0; i < 120; i++)
    sep << "-";
  sep << std::endl;
  out << toString(gress) << " MAU Features by Stage" << std::endl;

  // Table columns
  heading << sep.str();
  heading << "|" << boost::format("%=10s") % "Stage"
    << "|" << boost::format("%=10s") % "Exact"
    << "|" << boost::format("%=10s") % "Ternary"
    << "|" << boost::format("%=16s") % "Statistics"
    << "|" << boost::format("%=16s") % "Meter"
    << "|" << boost::format("%=16s") % "Selector"
    << "|" << boost::format("%=16s") % "Stateful"
    << "|" << boost::format("%=16s") % "Dependency"
    << "|" << std::endl;
  heading << "|" << boost::format("%=10s") % "Number"
    << "|" << boost::format("%=10s") % " "
    << "|" << boost::format("%=10s") % " "
    << "|" << boost::format("%=16s") % " "
    << "|" << boost::format("%=16s") % "LPF or WRED"
    << "|" << boost::format("%=16s") % "(max words)"
    << "|" << boost::format("%=16s") % " "
    << "|" << boost::format("%=16s") % "to Previous"
    << "|" << std::endl;
  heading << sep.str();
  out << heading.str();

  for (int stage=0; stage < Device::numStages(); ++stage) {
    std::stringstream st, ex, tern, stats, lpf, sel, stfl, deps;
    st << boost::format("%2d") % (stage % Device::numStages());
    if (stage_has_feature(gress, stage, HAS_EXACT)) { ex << "Yes"; } else { ex << "No"; }
    if (stage_has_feature(gress, stage, HAS_TCAM)) { tern << "Yes"; } else { tern << "No"; }
    if (stage_has_feature(gress, stage, HAS_STATS)) { stats << "Yes"; } else { stats << "No"; }
    if (stage_has_feature(gress, stage, HAS_LPF_OR_WRED)) { lpf << "Yes"; } else { lpf << "No"; }
    if (stage_has_feature(gress, stage, HAS_SEL)) {
      sel << "Yes (" << get_max_selector_words(gress, stage) << ")";
    } else {
      sel << "No (0)";
    }
    if (stage_has_feature(gress, stage, HAS_STFUL)) { stfl << "Yes"; } else { stfl << "No"; }

    mau_dep_t dep = get_dependency_for_gress_stage(gress, stage);
    deps << dep;

    std::stringstream line;
    line << "|" << boost::format("%=10s") % st.str()
      << "|" << boost::format("%=10s") % ex.str()
      << "|" << boost::format("%=10s") % tern.str()
      << "|" << boost::format("%=16s") % stats.str()
      << "|" << boost::format("%=16s") % lpf.str()
      << "|" << boost::format("%=16s") % sel.str()
      << "|" << boost::format("%=16s") % stfl.str()
      << "|" << boost::format("%=16s") % deps.str()
      << "|" << std::endl;
    out << line.str();
  }
  out << sep.str() << std::endl;
}

void MauFeatures::print_latency(std::ostream& out, gress_t gress) const {
  std::stringstream heading;
  std::stringstream sep;
  for (size_t i = 0; i < 75; i++)
    sep << "-";
  sep << std::endl;

  // Table columns
  heading << sep.str();
  heading << "|" << boost::format("%=10s") % "Stage"
    << "|" << boost::format("%=10s") % "Clock"
    << "|" << boost::format("%=16s") % "Predication"
    << "|" << boost::format("%=16s") % "Dependency"
    << "|" << boost::format("%=16s") % "Cycles Add"
    << "|" << std::endl;
  heading << "|" << boost::format("%=10s") % "Number"
    << "|" << boost::format("%=10s") % "Cycles"
    << "|" << boost::format("%=16s") % "Cycle"
    << "|" << boost::format("%=16s") % "to Previous"
    << "|" << boost::format("%=16s") % "to Latency"
    << "|" << std::endl;

  heading << sep.str();
  out << toString(gress) << " MAU Latency" << std::endl;
  out << heading.str();

  auto& spec = Device::mauPowerSpec();
  for (int stage=0; stage < Device::numStages(); ++stage) {
    mau_dep_t dep = get_dependency_for_gress_stage(gress, stage);
    int stage_latency = compute_stage_latency(gress, stage);
    int pred_cycle = compute_pred_cycle(gress, stage);

    std::stringstream st, clk, pred, deps, add;

    int add_to_lat = stage_latency;
    if (dep == DEP_CONCURRENT) {
      add_to_lat = spec.get_concurrent_latency_contribution();
    } else if (dep == DEP_ACTION) {
      add_to_lat = spec.get_action_latency_contribution();
    }
    deps << dep;

    st << boost::format("%2d") % (stage % Device::numStages());
    clk << boost::format("%2d") % stage_latency;
    pred << boost::format("%2d") % pred_cycle;
    add << boost::format("%2d") % add_to_lat;

    std::stringstream line;
    line << "|" << boost::format("%=10s") % st.str()
      << "|" << boost::format("%=10s") % clk.str()
      << "|" << boost::format("%=16s") % pred.str()
      << "|" << boost::format("%=16s") % deps.str()
      << "|" << boost::format("%=16s") % add.str()
      << "|" << std::endl;
    out << line.str();
  }
  out << sep.str();
  out << "Total latency for " << toString(gress) << ": ";
  out << compute_pipe_latency(gress) << std::endl << std::endl;
}

void MauFeatures::log_json_stage_characteristics(gress_t g, PowerLogging* logger) const {
  using StageChar = PowerLogging::StageCharacteristics;
  using Features = PowerLogging::Features;
  auto& spec = Device::mauPowerSpec();
  for (int stage=0; stage < Device::numStages(); ++stage) {
    mau_dep_t dep = get_dependency_for_gress_stage(g, stage);
    int stage_latency = compute_stage_latency(g, stage);
    int pred_cycle = compute_pred_cycle(g, stage);
    int add_to_lat = stage_latency;
    if (dep == DEP_CONCURRENT) {
      add_to_lat = spec.get_concurrent_latency_contribution();
    } else if (dep == DEP_ACTION) {
      add_to_lat = spec.get_action_latency_contribution();
    }

    auto *featuresJ = new Features(stage_has_feature(g, stage, HAS_EXACT),
                                   stage_has_feature(g, stage, HAS_LPF_OR_WRED),
                                   stage_has_feature(g, stage, HAS_SEL),
                                   stage_has_feature(g, stage, HAS_STFUL),
                                   stage_has_feature(g, stage, HAS_STATS),
                                   stage_has_feature(g, stage, HAS_TCAM),
                                   new int(get_max_selector_words(g, stage)));

    bool ext = stage_has_chained_feature(g, stage, HAS_EXACT) &&
      !stage_has_feature(g, stage, HAS_EXACT);
    bool tcm = stage_has_chained_feature(g, stage, HAS_TCAM) &&
      !stage_has_feature(g, stage, HAS_TCAM);
    bool sts = stage_has_chained_feature(g, stage, HAS_STATS) &&
      !stage_has_feature(g, stage, HAS_STATS);
    bool lpf = stage_has_chained_feature(g, stage, HAS_LPF_OR_WRED) &&
      !stage_has_feature(g, stage, HAS_LPF_OR_WRED);
    bool sel = stage_has_chained_feature(g, stage, HAS_SEL) &&
      !stage_has_feature(g, stage, HAS_SEL);
    bool stful = stage_has_chained_feature(g, stage, HAS_STFUL) &&
      !stage_has_feature(g, stage, HAS_STFUL);

    std::string dep_str(toString(dep));
    std::string gr_str(toString(g));

    auto *featuresBJ = new Features(ext, lpf, sel, stful, sts, tcm, 0);
    auto *stageJ = new StageChar(
      stage_latency,
      add_to_lat,
      dep_str,
      featuresJ,
      featuresBJ,
      gr_str,
      pred_cycle,
      stage);
    logger->append_stage_characteristics(stageJ);
  }
}

bool MprSettings::need_to_emit(lut_t type, int stage) const {
  const dyn_vector<int> * to_use = nullptr;
  if (type == NEXT_TABLE_LUT)
    to_use = &mpr_next_table_[stage];
  else if (type == GLOB_EXEC_LUT)
    to_use = &mpr_global_exec_[stage];
  else if (type == LONG_BRANCH_LUT)
    to_use = &mpr_long_branch_[stage];

  if (to_use) {
    for (auto v : *to_use) {
      if (v != 0)
        return true;
  } }
  return false;
}

std::ostream& MprSettings::emit_stage_asm(std::ostream& out, int stage) const {
  if (mau_features_.get_dependency_for_gress_stage(gress_, stage+1) == DEP_MATCH) {
    BUG_CHECK(get_mpr_bus_dep_glob_exec(stage) == 0, "mpr_bus_dep_glob_exec not zero");
    BUG_CHECK(get_mpr_bus_dep_long_brch(stage) == 0, "mpr_bus_dep_long_brch not zero");
  } else {
    BUG_CHECK(get_mpr_bus_dep_glob_exec(stage) == glob_exec_use[stage],
              "mpr_bus_dep_glob_exec does not match glob_exec_use");
    BUG_CHECK(get_mpr_bus_dep_long_brch(stage) == long_branch_use[stage],
              "mpr_bus_dep_long_brch does not match long_branch_use"); }
  out << "  mpr_stage_id: " << get_mpr_stage(stage) << std::endl;
  out << "  mpr_bus_dep_glob_exec: 0x" << hex(get_mpr_bus_dep_glob_exec(stage)) << std::endl;
  out << "  mpr_bus_dep_long_brch: 0x" << hex(get_mpr_bus_dep_long_brch(stage)) << std::endl;
  out << "  mpr_always_run: 0x" << hex(get_mpr_always_run_for_stage(stage)) << std::endl;
  if (need_to_emit(NEXT_TABLE_LUT, stage)) {
    out << "  mpr_next_table_lut: " << std::endl;
    for (int id=0; id < Memories::LOGICAL_TABLES; ++id) {
      if (get_mpr_next_table(stage, id))
        out << "    " << id << ": 0x" << hex(get_mpr_next_table(stage, id)) << std::endl;
  } }
  if (need_to_emit(GLOB_EXEC_LUT, stage)) {
    out << "  mpr_glob_exec_lut: " << std::endl;
    for (int bit=0; bit < Memories::LOGICAL_TABLES; ++bit) {
      if (get_mpr_global_exec(stage, bit))
        out << "    " << bit << ": 0x" << hex(get_mpr_global_exec(stage, bit)) << std::endl;
  } }
  if (need_to_emit(LONG_BRANCH_LUT, stage)) {
    out << "  mpr_long_brch_lut: " << std::endl;
    for (int id=0; id < Device::numLongBranchTags(); ++id) {
      if (get_mpr_long_branch(stage, id))
        out << "    " << id << ": 0x" << hex(get_mpr_long_branch(stage, id)) << std::endl;
  } }
  return out;
}

MprSettings::MprSettings(gress_t gress, MauFeatures &mf) : gress_(gress), mau_features_(mf) {}

void MprSettings::set_mpr_stage(int stage, int mpr_stage) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  mpr_stage_id_[stage] = mpr_stage;
}

int MprSettings::get_mpr_stage(int stage) const {
  return mpr_stage_id_[stage];
}

void MprSettings::set_mpr_always_run(int stage, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  mpr_always_run_[stage] = id_vector;
}

void MprSettings::set_or_mpr_always_run(int stage, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  mpr_always_run_[stage] |= id_vector;
}

int MprSettings::get_mpr_always_run_for_stage(int stage) const {
  return mpr_always_run_[stage];
}

void MprSettings::set_mpr_next_table(int stage, int logical_id, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  BUG_CHECK(logical_id >= 0, "Invalid logical_id %d", logical_id);
  mpr_next_table_[stage][logical_id] = id_vector;
}

void MprSettings::set_or_mpr_next_table(int stage, int logical_id, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  BUG_CHECK(logical_id >= 0, "Invalid logical_id %d", logical_id);
  mpr_next_table_[stage][logical_id] |= id_vector;
}

int MprSettings::get_mpr_next_table(int stage, int logical_id) const {
  return mpr_next_table_[stage][logical_id];
}

void MprSettings::set_mpr_global_exec(int stage, int exec_bit, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  BUG_CHECK(exec_bit >= 0, "Invalid execute bit %d", exec_bit);
  mpr_global_exec_[stage][exec_bit] = id_vector;
}

void MprSettings::set_or_mpr_global_exec(int stage, int exec_bit, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  BUG_CHECK(exec_bit >= 0, "Invalid execute bit %d", exec_bit);
  mpr_global_exec_[stage][exec_bit] |= id_vector;
}

int MprSettings::get_mpr_global_exec(int stage, int exec_bit) const {
  return mpr_global_exec_[stage][exec_bit];
}

void MprSettings::set_mpr_long_branch(int stage, int tag_id, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  BUG_CHECK(tag_id >= 0, "Invalid long branch tag_id %d", tag_id);
  mpr_long_branch_[stage][tag_id] = id_vector;
}

int MprSettings::get_mpr_long_branch(int stage, int tag_id) const {
  return mpr_long_branch_[stage][tag_id];
}

void MprSettings::set_or_mpr_long_branch(int stage, int tag_id, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  BUG_CHECK(tag_id >= 0, "Invalid long branch tag_id %d", tag_id);
  mpr_long_branch_[stage][tag_id] |= id_vector;
}

void MprSettings::set_mpr_bus_dep_glob_exec(int stage, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  mpr_bus_dep_glob_exec_[stage] = id_vector;
}

void MprSettings::set_or_mpr_bus_dep_glob_exec(int stage, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  mpr_bus_dep_glob_exec_[stage] |= id_vector;
}

int MprSettings::get_mpr_bus_dep_glob_exec(int stage) const {
  return mpr_bus_dep_glob_exec_[stage];
}

void MprSettings::set_mpr_bus_dep_long_brch(int stage, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  mpr_bus_dep_long_brch_[stage] = id_vector;
}

void MprSettings::set_or_mpr_bus_dep_long_brch(int stage, int id_vector) {
  BUG_CHECK(stage >= 0, "Invalid stage %d", stage);
  mpr_bus_dep_long_brch_[stage] |= id_vector;
}

int MprSettings::get_mpr_bus_dep_long_brch(int stage) const {
  return mpr_bus_dep_long_brch_[stage];
}

void MprSettings::print_data(std::ostream &out, int cols, std::string id_name,
                             std::vector<int> data, bool use_bin) const {
  std::stringstream heading;
  std::stringstream sep;

  int log_tbl = Memories::LOGICAL_TABLES;
  while ((log_tbl % 4) != 0) {
    ++log_tbl; }  // for formatting purposes
  std::stringstream sl;
  sl << "%=" << (10 + log_tbl) << "s";

  // line is | 12 | 12 | 25 |
  int line_length = 12*cols + cols + log_tbl - 1;
  for (int i=0; i < line_length; ++i)
    sep << "-";
  sep << std::endl;

  // Table columns
  heading << sep.str();
  if (cols == 2) {
    heading << "|" << boost::format("%=12s") % "Stage"
      << "|" << boost::format(sl.str()) % id_name
      << "|" << std::endl;
  } else if (cols == 3) {
    heading << "|" << boost::format("%=12s") % "Stage"
      << "|" << boost::format("%=12s") % id_name
      << "|" << boost::format(sl.str()) % "Active"
      << "|" << std::endl;
  } else {
    return;
  }
  heading << sep.str();

  bool set_something = false;
  std::stringstream all_data;
  int elem = 0;
  int line[3];
  for (auto d : data) {
    line[elem % cols] = d;
    if ((elem % cols) == (cols - 1)) {  // last column of line
      std::stringstream vec;
      int bin = 0;
      int start_bit = 1 << (Memories::LOGICAL_TABLES - 1);
      while (start_bit > 0) {
        if (bin && (bin % 4) == 0)
          vec << "_";
        if (start_bit & d)
          vec << "1";
        else
          vec << "0";
        ++bin;
        start_bit = start_bit >> 1;
      }
      if (d != 0) {
        set_something = true;
        for (int c=0; c < (cols - 1); ++c) {
          std::stringstream st;
          st << boost::format("%2d") % (line[c]);
          all_data << "|" << boost::format("%=12s") % st.str();
        }
        if (!use_bin) {
          std::stringstream st;
          st << boost::format("%2d") % (d);
          all_data << "|" << boost::format(sl.str()) % st.str() << "|" << std::endl;
        } else {
          all_data << "|" << boost::format(sl.str()) % vec.str() << "|" << std::endl;
        }
      }
      line[0] = 0;
      line[1] = 0;
      line[2] = 0;
    }
    ++elem;
  }
  if (!set_something) {
    for (int c=0; c < (cols - 1); ++c) {
      all_data << "|" << boost::format("%=12s") % "-"; }
    all_data << "|" << boost::format(sl.str()) % "-" << "|" << std::endl;
  }
  out << heading.str() << all_data.str() << sep.str();
}

std::ostream& operator<<(std::ostream &out, const MprSettings &m) {
  out << "MPR settings " << m.gress_ << ":" << std::endl;
  std::vector<int> vec;
  for (int s=0; s < Device::numStages(); ++s) {
    vec.push_back(s);
    vec.push_back(m.mpr_stage_id_.at(s)); }
  out << "MPR Stage" << std::endl;
  m.print_data(out, 2, "Stage ID", vec, false);
  vec.clear();
  for (int s=0; s < Device::numStages(); ++s) {
    for (int id=0; id < Memories::LOGICAL_TABLES; ++id) {
      vec.push_back(s);
      vec.push_back(id);
      vec.push_back(m.mpr_next_table_[s][id]); } }
  out << "MPR Next Table" << std::endl;
  m.print_data(out, 3, "Logical ID", vec, true);
  vec.clear();
  for (int s=0; s < Device::numStages(); ++s) {
    for (int id=0; id < Memories::LOGICAL_TABLES; ++id) {
      vec.push_back(s);
      vec.push_back(id);
      vec.push_back(m.mpr_global_exec_[s][id]); } }
  out << "MPR Global Exec" << std::endl;
  m.print_data(out, 3, "Execute Bit", vec, true);
  vec.clear();
  for (int s=0; s < Device::numStages(); ++s) {
    for (int id=0; id < Device::numLongBranchTags(); ++id) {
      vec.push_back(s);
      vec.push_back(id);
      vec.push_back(m.mpr_long_branch_[s][id]); } }
  out << "MPR Long Branch" << std::endl;
  m.print_data(out, 3, "Tag ID", vec, true);
  vec.clear();
  for (int s=0; s < Device::numStages(); ++s) {
    vec.push_back(s);
    vec.push_back(m.mpr_always_run_.at(s)); }
  out << "MPR Always Run" << std::endl;
  m.print_data(out, 2, "Active", vec, true);
  vec.clear();
  for (int s=0; s < Device::numStages(); ++s) {
    vec.push_back(s);
    vec.push_back(m.get_mpr_bus_dep_glob_exec(s)); }
  out << "MPR Bus Dep Glob Exec" << std::endl;
  m.print_data(out, 2, "Bus Dep", vec, true);
  vec.clear();
  for (int s=0; s < Device::numStages(); ++s) {
    vec.push_back(s);
    vec.push_back(m.get_mpr_bus_dep_long_brch(s)); }
  out << "MPR Bus Dep Long Branch" << std::endl;
  m.print_data(out, 2, "Bus Dep", vec, true);
  return out;
}

/**
  * Computes the power consumed by a single table in the specified number of pipelines.
  * Note that the result of this function should not be exposed to the user
  * in logging.  (Yes, it can experimentally derived fairly easily...)
  * @param num_pipes The number of pipelines to consider the table in.
  * @return An estimate for the power consumption (in Watts) based on the
  *         table's access pattern.
  */
double PowerMemoryAccess::compute_table_power(int num_pipes) const {
  double power = 0.0;
  auto& spec = Device::mauPowerSpec();
  power += (ram_read * spec.get_ram_read_power());
  power += (ram_write * spec.get_ram_write_power());
  power += (tcam_read * spec.get_tcam_search_power());
  power += (map_ram_read * spec.get_map_ram_read_power());
  power += (map_ram_write * spec.get_map_ram_write_power());
  power += (deferred_ram_read * spec.get_deferred_ram_read_power());
  power += (deferred_ram_write * spec.get_deferred_ram_write_power());
  return num_pipes * power;  // Account for power in all pipes
}

/**
  * Normalize power units to a unitless weight value.
  * The result of this function is the one that can be exposed to the user.
  */
double PowerMemoryAccess::compute_table_weight(double computed_power,
                                               int num_pipes) const {
  auto& spec = Device::mauPowerSpec();
  double min_power_access = spec.get_ram_read_power();
  min_power_access = std::min(min_power_access, spec.get_ram_write_power());
  min_power_access = std::min(min_power_access, spec.get_tcam_search_power());
  min_power_access = std::min(min_power_access, spec.get_map_ram_read_power());
  min_power_access = std::min(min_power_access, spec.get_map_ram_write_power());
  min_power_access = std::min(min_power_access, spec.get_deferred_ram_read_power());
  min_power_access = std::min(min_power_access, spec.get_deferred_ram_write_power());

  if (min_power_access > 0 && num_pipes > 0) {
    double normalized_weight = computed_power / min_power_access;
    // Make normalized weight for single pipe.
    return normalized_weight / num_pipes;
  }
  return 0.0;
}

void PowerMemoryAccess::log_json_memories(PowerLogging::StageDetails* sd) const {
  using Memories = PowerLogging::StageDetails::Memories;
  if (ram_read > 0)
    sd->append(new Memories("read", "sram", ram_read));
  if (ram_write > 0)
    sd->append(new Memories("write", "sram", ram_write));
  if (tcam_read > 0)
    sd->append(new Memories("search", "tcam", tcam_read));
  if (map_ram_read > 0)
    sd->append(new Memories("read", "map_ram", map_ram_read));
  if (map_ram_write > 0)
    sd->append(new Memories("write", "map_ram", map_ram_write));
  if (deferred_ram_read > 0)
    sd->append(new Memories("read", "deferred_ram", deferred_ram_read));
  if (deferred_ram_write > 0)
    sd->append(new Memories("write", "deferred_ram", deferred_ram_write));
}

}  // end namespace MauPower
