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

#ifndef BF_P4C_PHV_ALLOC_SETTING_H_
#define BF_P4C_PHV_ALLOC_SETTING_H_

namespace PHV {

// AllocSetting holds various alloc settings.
struct AllocSetting {
    /// trivial allocation mode that
    /// (1) do minimal packing, i.e., pack fieldslices only when unavoidable.
    /// (2) assume that there were infinite number of containers.
    /// (3) no metadata or dark initialization, as if no_code_change mode is enabled.
    bool trivial_alloc = false;
    bool no_code_change = false;              // true if disable metadata and dark init.
    bool physical_liverange_overlay = false;  // true if allow physical liverange overlay.
    bool limit_tmp_creation = false;          // true if intermediate tmp value are limited.
    bool single_gress_parser_group = false;   // true if PragmaParserGroupMonogress enabled.
    bool prioritize_ara_inits = false;        // true if PragmaPrioritizeARAinits enabled.
    bool physical_stage_trivial = false;      // true if minstage based trivial alloc failed.
};

}  // namespace PHV

#endif /* BF_P4C_PHV_ALLOC_SETTING_H_ */
