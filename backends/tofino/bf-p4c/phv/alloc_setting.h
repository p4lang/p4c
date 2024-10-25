/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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
