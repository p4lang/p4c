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

#ifndef BF_P4C_MAU_MAU_POWER_H_
#define BF_P4C_MAU_MAU_POWER_H_

#include <fstream>
#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "ir/ir.h"
#include "ir/unique_id.h"
#include "bf-p4c/device.h"
#include "lib/dyn_vector.h"
#include "power_schema.h"

namespace MauPower {

using namespace P4;

enum mau_dep_t {DEP_CONCURRENT = 0, DEP_ACTION = 1, DEP_MATCH = 2};
enum stage_feature_t {HAS_EXACT = 0, HAS_TCAM = 1, HAS_STATS = 2,
                      HAS_SEL = 3, HAS_LPF_OR_WRED = 4, HAS_STFUL = 5,
                      WIDE_SEL = 6};
enum lut_t {NEXT_TABLE_LUT = 0, GLOB_EXEC_LUT = 1, LONG_BRANCH_LUT = 2};

std::ostream &operator<<(std::ostream &, mau_dep_t);
// The internal encoding of stage numbers is:
//      0 to n-1 for ingress stages
//      n to 2n-1 for egress stages
//      2n to 3n-1 for ghost stages
// However, note that function signatures assume the stage argument
// runs from 0 to n-1.  This is checked.
std::string float2str(double d);

/**
  * A class to represent MAU stage characteristics, such as:
  * - what features are in use in each MAU stage in each gress
  * - the dependency to the previous stage for each MAU stage in each gress
  */
class MauFeatures {
 public:
  static const int kNumberGress = GRESS_T_COUNT;
  using PowerLogging = Logging::Power_Schema_Logger;
  // For calculating latencies
  // map from gress and stage number to Boolean indicating if resource type is in use.
  bitvec has_exact_[kNumberGress];
  // has_tcam_ would also include ternary result buses in ram array.
  bitvec has_tcam_[kNumberGress];
  bitvec has_meter_lpf_or_wred_[kNumberGress];
  bitvec has_selector_[kNumberGress];
  dyn_vector<int> max_selector_words_[kNumberGress];  // max across tables in stage
  bitvec has_stateful_[kNumberGress];
  bitvec has_stats_[kNumberGress];

  // map from UniqueId to Boolean indicating
  // if it will run at EOP.
  // The UniqueId is the attached table's UniqueId.
  std::map<UniqueId, bool> counter_runs_at_eop_;
  std::map<UniqueId, bool> meter_runs_at_eop_;
  // for keeping track if have LPF or WRED 'meter'
  std::map<UniqueId, bool> meter_is_lpf_or_wred_;
  // stores maximum number of selector members
  // (more than 120 requires multiple RAM words)
  std::map<UniqueId, int> selector_group_size_;

  // Maps stage number to vector of logical tables found in the stage
  dyn_vector<std::vector<const IR::MAU::Table*>> stage_to_tables_[kNumberGress];
  // Maps UniqueId to stage number it's in.
  std::map<UniqueId, int> table_to_stage_;
  // Maps UniqueId back to the table
  std::map<UniqueId, const IR::MAU::Table*> uid_to_table_;

 private:
  // Maps stage number to its dependency type to previous stage.
  // Note the encoding is as described above.
  std::map<int, mau_dep_t> stage_dep_to_previous_[kNumberGress];

 public:
  /**
    * @param gress The thread of compute.
    * @param stage The MAU stage number, running 0 to n-1.
    * @param feature The MAU feature.
    * @return Boolean indicating if a particular stage has a given feature.
    */
  bool stage_has_feature(gress_t gress, int stage, stage_feature_t feature) const;
  /**
    * @param gress The thread of compute.
    * @param stage The MAU stage number, running 0 to n-1.
    * @return The maximum number of RAM words required to hold the largest
    *         selector group.  For example, a group size of 120 results in
    *         1 being returned.  A group size of 121 results in 2 being returned.
    */
  int get_max_selector_words(gress_t gress, int stage) const;
  /**
    * @param g     The thread of compute.
    * @param stage The MAU stage number, running 0 to n-1.
    * @return Stage dependency type to the previous MAU stage.
    */
  mau_dep_t get_dependency_for_gress_stage(gress_t g, int stage) const;
  void set_dependency_for_gress_stage(gress_t g, int stage, mau_dep_t dep);
  /**
    * Attempts to convert a MAU stage dependency to match dependent.
    * If a stage can be changed, the change is performed and this function returns true.
    * Otherwise, it returns false.
    * @return Boolean indicating if a MAU stage in any thread was converted
    *         to match dependent.
    */
  bool try_convert_to_match_dep();
  /**
    * Removes concurrent dependency type if dealing with Tofino2 or later.
    */
  void update_deps_for_device();

  /**
    * @param gress The thread of compute.
    * @return A specific pipeline latency (in clock cycles).
    */
  int compute_pipe_latency(gress_t gress) const;
  /**
    * @param gress The thread of compute.
    * @param stage The MAU stage number, running 0 to n-1.
    * @return The stage latency of a specific MAU gress-stage (in clock cycles).
    */
  int compute_stage_latency(gress_t gress, int stage) const;
  /**
    * @param gress The thread of compute.
    * @param stage The MAU stage number, running 0 to n-1.
    * @return The predication cycle of a specific MAU gress-stage (in clock cycles)
    */
  int compute_pred_cycle(gress_t gress, int stage) const;
  /**
    * @param gress The thread of compute.
    * @param stage The MAU stage number, running 0 to n-1.
    * @param feature The MAU feature.
    * @return Boolean indicating if a particular stage should be considered as
    *         having a particular feature for timing balance reasons.
    *         This is only applicable when the stage is *not* separated by
    *         match dependencies with its adjacent stages.
    */
  bool stage_has_chained_feature(gress_t gress, int stage, stage_feature_t feature) const;
  /**
    * Produces lovely text tables showing features and dependencies.
    */
  void print_features(std::ostream& out, gress_t gress) const;
  /**
    * Produces lovely text tables showing latencies and dependencies.
    */
  void print_latency(std::ostream& out, gress_t gress) const;
  /**
    * Produces the power.json output.
    */
  void log_json_stage_characteristics(gress_t g, PowerLogging* logger) const;
  /**
    * Emits information that needs to be communicated to assembly for
    * register configuration.  Currently, this is the MAU stage dependencies.
    */
  std::ostream& emit_dep_asm(std::ostream& out, gress_t g, int stage) const;
  /**
    * Return true if some asm code is required for the stage for correct behavior,
    * even if no tables are present
    */
  bool requires_dep_asm(gress_t g, int stage) const;
  /**
    * Returns true if there are tables remaining in a particular gress
    * in the start stage until the last MAU stage.
    */
  bool are_there_more_tables(gress_t gress, int start_stage) const;
};

/**
  * A class to represent the match-power reduction (MPR) settings that will need
  * to be passed to assembly for register configuration.
  * This is only relevant for Tofino2 and beyond.
  * A separate MprSettings object is created for each gress, so stage parameters
  * in this class run 0 to n-1.  They do not use the encoding mentioned above.
  */
class MprSettings {
 public:
  const gress_t gress_;
  MauFeatures   &mau_features_;
  MprSettings(gress_t gress, MauFeatures &);
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param mpr_stage This stage, if it is match dependent, or the preceding match-dependent
    *                  stage otherwise.  The next_table, glob_exec, and long_branch input will be
    *                  drawn from the output of the stage before this.
    */
  void set_mpr_stage(int stage, int mpr_stage);
  int get_mpr_stage(int stage) const;
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param logical_id The logical ID number that will activate the id_vector.
    *               Allowed values are [0:15].
    * @param id_vector A 16-bit activation vector that says what logical tables
    *                  in the current MAU stage to power on.
    */
  void set_mpr_next_table(int stage, int logical_id, int id_vector);
  void set_or_mpr_next_table(int stage, int logical_id, int id_vector);
  int get_mpr_next_table(int stage, int logical_id) const;
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param exec_bit The global execute bit number that will activate the id_vector.
    *                 Allowed values are [0:15].
    * @param id_vector A 16-bit activation vector that says what logical tables
    *                  in the current MAU stage to power on.
    */
  void set_mpr_global_exec(int stage, int exec_bit, int id_vector);
  void set_or_mpr_global_exec(int stage, int exec_bit, int id_vector);
  int get_mpr_global_exec(int stage, int exec_bit) const;
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param tag_id The long branch tag ID number that will activate the id_vector.
    *               Allowed values are [0:7].
    * @param id_vector A 16-bit activation vector that says what logical tables
    *                  in the current MAU stage to power on.
    */
  void set_mpr_long_branch(int stage, int tag_id, int id_vector);
  int get_mpr_long_branch(int stage, int tag_id) const;
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param tag_id The long branch tag ID number that will activate the id_vector.
    *               Allowed values are [0:7].
    * @param id_vector A 16-bit activation vector that says what logical tables
    *                  in the current MAU stage to power on.  This value will
    *                  be ORed into the existing value, if it exists.
    */
  void set_or_mpr_long_branch(int stage, int tag_id, int id_vector);
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param id_vector A 16-bit activation vector that says what logical tables
    *                  in the current MAU stage are always power on.
    */
  void set_mpr_always_run(int stage, int id_vector);
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param id_vector A 16-bit activation vector that says what logical tables
    *                  in the current MAU stage are always power on.  This value
    *                  will be ORed into the existing value, if it exists.
    */
  void set_or_mpr_always_run(int stage, int id_vector);
  int get_mpr_always_run_for_stage(int stage) const;
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param id_vector A 16-bit vector indicating if the *following* stage is
    *                  action dependent for the given global execute activation
    *                  bit.  A value of '1' indicates that the input global
    *                  execute bit should be passed through unchanged.  If the
    *                  next stage is match dependent, this bit will be updated
    *                  by the current stage.
    *                  Note that global execute is not per-gress, so this
    *                  value will be merged with the other gresses prior to
    *                  writing configuration registers.
    */
  void set_mpr_bus_dep_glob_exec(int stage, int id_vector);
  void set_or_mpr_bus_dep_glob_exec(int stage, int id_vector);
  int get_mpr_bus_dep_glob_exec(int stage) const;
  /**
    * @param stage The MAU stage number where the configuration will be written.
    * @param id_vector An 8-bit vector indicating if the *following* stage is
    *                  action dependent for the given long branch tag ID activation
    *                  bit.  A value of '1' indicates that the input long branch
    *                  tag ID execution bit should be passed through unchanged.
    *                  If the next stage is match dependent, this bit will be
    *                  updated by the current stage.
    *                  Note that long branch is not per-gress, so this
    *                  value will be merged with the other gresses prior to
    *                  writing configuration registers.
    */
  void set_mpr_bus_dep_long_brch(int stage, int id_vector);
  void set_or_mpr_bus_dep_long_brch(int stage, int id_vector);
  int get_mpr_bus_dep_long_brch(int stage) const;

  friend std::ostream& operator<<(std::ostream &out, const MprSettings &m);
  /**
    * @param type The type of look-up table (LUT).
    * @param stage The MAU stage number where the configuration will be written.
    * @return A Boolean indicating if any of the LUT values are non-zero.
    *         Non-zero values need to be written to the assembly file.
    */
  bool need_to_emit(lut_t type, int stage) const;
  std::ostream& emit_stage_asm(std::ostream &out, int stage) const;

 private:
  /**
    * map from stage number to the MAU stage to retrieve next table / global exec / long branch
    * information from.  When match dependent, the bus to be used is from the same MAU stage.
    * When action dependent, it is the stage on which there was a prior match dependency.
    * This stage ID forms the basis for the view of the 'logical id' key for the following maps.
    * For example, if stage 2 is action dependent on 1, but there is a match
    * dependency to 0, the activation for logical table 0 should be the logical
    * tables in stage 2 that can be reached by logical table 0 in MAU 0.
    * See example programming in Section 6.4.2.3.5 (Match Power Reduction Example)
    * in the Tofino2 Match Action Unit Micro Architecture document.
    */
  dyn_vector<int> mpr_stage_id_;

  /**
    * map from stage number to map from logical table id to a
    * logical table activation vector (16 bits).  This specifies
    * which logical tables will be turned on in the given stage
    * if the input logical table is enabled in this stage when match dependent or
    * in a previous stage when action dependent.
    * (This coincides with local execute as well.)
    * Inner map is at most 16 entries for each possible logical table ID in a stage.
    */
  dyn_vector<dyn_vector<int>> mpr_next_table_;

  /**
    * map from stage number to map from global execute bit (of previous
    * match dependent input) to a logical table activation vector (16 bits).
    * This specifies which logical tables will be turned on in this stage
    * if the previous match dependent global execute bit is enabled.
    * Inner map is at most 16 entries for each possible global execute bit in a stage.
    */
  dyn_vector<dyn_vector<int>> mpr_global_exec_;

  /**
    * map from stage number to map from long branch tag ID to a logical table
    * activation vector (16 bits).  This specifies which tables will be turned
    * on in this stage when the long branch tag ID is seen at the input.
    * Inner map is at most 8 entries for the 8 tags.
    */
  dyn_vector<dyn_vector<int>> mpr_long_branch_;

  /**
    * map from stage number to activation vector (16 bits), saying which
    * logical tables in a given stage are always run.
    */
  dyn_vector<int> mpr_always_run_;

  /**
    * map from stage number to 16-bit ID vector, saying whether the following
    * stage is action dependent on the current stage (bit in vector set to 1).
    * When the value is 1, the input value of a particular bit will be passed
    * through the current stage unchanged (since it will not have resolved
    * due to the action dependency).  A zero value means the current stage will
    * update the particular global execute bit position.
    */
  dyn_vector<int> mpr_bus_dep_glob_exec_;

  /**
    * map from stage number to 8-bit ID vector, saying whether the following
    * stage is action dependent on the current stage (bit in vector set to 1).
    * When the value is 1, the input value of a particular bit will be passed
    * through the current stage unchanged (since it will not have resolved
    * due to the action dependency).  A zero value means the current stage will
    * update the particular long branch tag ID bit position.
    */
  dyn_vector<int> mpr_bus_dep_long_brch_;

  void print_data(std::ostream &out, int cols, std::string id_name,
                  std::vector<int> data, bool use_bin) const;

 public:
  /* for each stage, which mpr_glob_exec bits output by this stage are needed by later stages.
   * for action dependent stages, this will be identical to the previous match dependent stage.
   */
  dyn_vector<int> glob_exec_use;
  /* for each stage, which mpr_long_branch tags output by this stage are needed by later stages.
   * for action dependent stages, this will be identical to the previous match dependent stage.
   */
  dyn_vector<int> long_branch_use;
};


/*
 * rams_read
 *    The total number of RAMs read.
 * rams_write
 *    The total number of RAMs written.  Only attached synth-2-port
 *    resources can write RAMs.
 * tcam_read
 *    The total number of TCAMs read.  Ternary tables have to read
 *    all TCAMs.
 * map_rams_read
 *    The total number of MapRAMs read.  MapRAMs are read by synth-2-port,
 *    idletime, and meter color.
 * map_rams_write
 *    The total number of MapRAMs written.  MapRAMs are written by
 *    synth-2-port, idletime, and meter color.
 * deferred_rams_read
 *    The total number of deferred RAMs read.  Deferred RAMs are read
 *    by meters and counters that run at EOP time.
 * deferred_rams_write
 *    The total number of DeferredRAMs written.  Deferred RAMs are written
 *    by meters and counters that run at EOP time.
 */
struct PowerMemoryAccess {
  using PowerLogging = Logging::Power_Schema_Logger;
  int ram_read = 0;
  int ram_write = 0;
  int tcam_read = 0;
  int map_ram_read = 0;
  int map_ram_write = 0;
  int deferred_ram_read = 0;
  int deferred_ram_write = 0;

  PowerMemoryAccess() :
    ram_read(0), ram_write(0),
    tcam_read(0),
    map_ram_read(0), map_ram_write(0),
    deferred_ram_read(0), deferred_ram_write(0) {}

  explicit PowerMemoryAccess(int ram_read, int ram_write,
                             int tcam_read,
                             int map_ram_read, int map_ram_write,
                             int deferred_ram_read, int deferred_ram_write) :
    ram_read(ram_read), ram_write(ram_write),
    tcam_read(tcam_read),
    map_ram_read(map_ram_read), map_ram_write(map_ram_write),
    deferred_ram_read(deferred_ram_read), deferred_ram_write(deferred_ram_write) {}

  friend std::ostream &operator<<(std::ostream &out, const PowerMemoryAccess &p) {
    out << "Memory access:" << std::endl;
    out << "   RAMs read " << p.ram_read << std::endl;
    out << "   RAMs write " << p.ram_write << std::endl;
    out << "   TCAMs read " << p.tcam_read << std::endl;
    out << "   MapRAMs read " << p.map_ram_read << std::endl;
    out << "   MapRAMs write " << p.map_ram_write << std::endl;
    out << "   Deferred RAMs read " << p.deferred_ram_read << std::endl;
    out << "   Deferred RAMs write " << p.deferred_ram_write << std::endl;
    return out;
  }

  PowerMemoryAccess &operator+=(const PowerMemoryAccess &p) {
    ram_read += p.ram_read;
    ram_write += p.ram_write;
    tcam_read += p.tcam_read;
    map_ram_read += p.map_ram_read;
    map_ram_write += p.map_ram_write;
    deferred_ram_read += p.deferred_ram_read;
    deferred_ram_write += p.deferred_ram_write;
    return *this;
  }

  PowerMemoryAccess operator+(const PowerMemoryAccess &p) const {
    PowerMemoryAccess rv = *this; rv += p; return rv; }

  /**
    * Computes the worst-case power consumed given parameters that indicate
    * the number of different memory types read and written.
    * This function is intended to be called per logical table in each stage,
    * where the parameters include the sum of memories accessed by the
    * match table and all its attached tables.
    * For example, if an exact match table has a directly attached counter,
    * the parameters should have non-zero values for rams_read, rams_write,
    * map_rams_read, and map_rams_write.  If the counter runs at EOP time,
    * the deferred_rams_read and deferred_rams_write should have non-zero
    * values as well.
    * Note that the value this function returns should not be exposed
    * to an end-user.
    *
    * @param num_pipes
    *    The number of pipes to consider this table in.
    * @return
    *    The total power consumed (in Watts).
    */
  double compute_table_power(int num_pipes) const;

  /**
    * Returns a normalized weight of a table's power contribution.
    * We do not generally want to expose the raw Watts used by a table, as
    * this is considered Barefoot secret.
    * @return
    *     The normalized weight of the power contribution of this table.
    *     Note that this is normalized to one pipeline.
    */
  double compute_table_weight(double computed_power, int num_pipes) const;
  // Sets up JSON logging information.
  void log_json_memories(PowerLogging::StageDetails*) const;
};

}  // end namespace MauPower

#endif /* BF_P4C_MAU_MAU_POWER_H_ */
