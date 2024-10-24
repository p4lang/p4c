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

#include "bf-p4c/test/utils/super_cluster_builder.h"

#include "bf-p4c/../p4c/lib/log.h"

////////////////////////////////////////////////////////////////////////////////
// Helpfull inline functions
////////////////////////////////////////////////////////////////////////////////
void print_error(std::string str) { LOGN(-1, "ERROR: SuperClusterBuilder :" + str); }

std::string get_token(std::istream &fs) {
    std::string token;

    fs >> token;

    return token;
}

int expect_token(std::istream &fs, std::string exp_token) {
    std::string token = get_token(fs);

    int ret = token.compare(exp_token);

    if (ret != 0)
        print_error("Unexpected token \"" + token +
                    "\" found during analyze_super_cluster. Expected: \"" + exp_token);
    return ret;
}

// Starts from a given index and finds first full set of "[...]"
// Returns 0 if this was sucessfull, 1 if nothing was found
int get_whole_parentheses(std::string &s, unsigned int start_from, unsigned int *lb,
                          unsigned int *ub) {
    int level = 1;
    unsigned int iter = start_from;
    unsigned int len = s.length();

    // Find first opening '['
    while (iter < len && s[iter] != '[') iter++;

    // Check the end of string
    if (iter >= len) return 1;

    // Set the new lb
    *lb = iter;

    // Move behind the first '['
    iter++;
    // Find the appropriate closing ']'
    while (iter < len && level != 0) {
        if (s[iter] == '[') level++;
        if (s[iter] == ']') level--;
        iter++;
    }

    // Set the upper bound - discount the last increment
    *ub = iter - 1;

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Class functions
////////////////////////////////////////////////////////////////////////////////
std::optional<PHV::SuperCluster *> SuperClusterBuilder::build_super_cluster(std::istream &scs) {
    // Set the member variable

    std::optional<PHV::SuperCluster *> sc = analyze_super_cluster(scs);

    // Return the SuperCluster
    return sc;
}

// Parses the input of the whole SuperCluster in format produced by the
// << operator on PHV::SuperCluster (can be found in phv/utils/utils.cpp)
std::optional<PHV::SuperCluster *> SuperClusterBuilder::analyze_super_cluster(std::istream &scs) {
    int ret = 0;         // Return value
    std::string t;       // Next token
    std::streampos pos;  // Keeping track of positions for returning

    std::optional<PHV::SuperCluster *> sc;
    /// Set of rotational clusters extracted
    ordered_set<const PHV::RotationalCluster *> clusters;
    /// Set of slice lists extracted
    ordered_set<PHV::SuperCluster::SliceList *> slice_lists;

    if ((ret = expect_token(scs, "SUPERCLUSTER")) != 0) return sc;
    if ((ret = expect_token(scs, "Uid:")) != 0) return sc;
    t = get_token(scs);

    if ((ret = expect_token(scs, "slice")) != 0) return sc;
    if ((ret = expect_token(scs, "lists:")) != 0) return sc;

    // Try to look ahead to determine if there are slice lists (not only "[ ]")
    if ((ret = expect_token(scs, "[")) != 0) return sc;
    pos = scs.tellg();
    t = get_token(scs);
    // If there were no slice lists continue
    // Otherwise go back and start parsing the slice lists
    if (t.compare("]") != 0) {
        scs.seekg(pos);
        bool next_list = true;

        // Repeat until there are lists to analyze
        while (next_list) {
            PHV::SuperCluster::SliceList *sl = analyze_slice_list(scs);
            if (sl == nullptr) {
                print_error("analyze_super_cluster() Failed while extracting slice lists");
                return sc;
            }
            slice_lists.insert(sl);
            // Look ahead to determine if there are any more
            pos = scs.tellg();
            t = get_token(scs);
            // "[" means there is another list, otherwise there is none
            if (t.compare("[") != 0) {
                next_list = false;
                scs.seekg(pos);
            }
        }
    }

    if ((ret = expect_token(scs, "rotational")) != 0) return sc;
    if ((ret = expect_token(scs, "clusters:")) != 0) return sc;

    // Try to look ahead to determine if there are rotational clusters (not only "{ }")
    pos = scs.tellg();
    t = get_token(scs);
    // If there are cluster analyze them
    if (t.compare("{") != 0) {
        scs.seekg(pos);
        // Do an empty getline just to get to the start of the next line
        std::getline(scs, t);
        // Repeat until the end of the file (for each rot. cluster)
        while (!scs.eof()) {
            PHV::RotationalCluster *rc = analyze_rotational_cluster(scs);
            if (rc == nullptr) {
                print_error("analyze_super_cluster() Failed while extracting rotational clusters");
                return sc;
            }
            // Ignore empty clusters
            if (!rc->clusters().empty()) {
                clusters.insert(rc);
            }
        }
        // Otherwise this should be the end
    } else {
        if ((ret = expect_token(scs, "}")) != 0) return sc;
    }

    sc = new PHV::SuperCluster(clusters, slice_lists);
    return sc;
}

// Parses the input of the whole SliceList in format produced by the
// << operator on PHV::SliceList (can be found in phv/utils/utils.cpp)
PHV::SuperCluster::SliceList *SuperClusterBuilder::analyze_slice_list(std::istream &scs) {
    std::string line;        // One line == one slice
    bool next_slice = true;  // Is there another slice?
    // The resulting SliceList
    PHV::SuperCluster::SliceList *sl = new PHV::SuperCluster::SliceList;
    PHV::FieldSlice *slice;

    // In this case each line is a single slice
    while (next_slice) {
        std::getline(scs, line);
        // If the line ends with the additional " ]" we know this is the last slice
        if (line.length() >= 2 && line[line.length() - 1] == ']' &&
            line[line.length() - 2] == ' ') {
            next_slice = false;
            // Remove the 2 extra characters
            line.resize(line.size() - 2);
        }

        // Analyze field slice
        slice = analyze_field_slice(line);
        if (slice == nullptr) {
            print_error("analyze_slice_list() Failed while extracting Field Slice");
            return nullptr;
        }

        // Add the slice into slice list
        sl->push_back(*slice);
    }

    return sl;
}

// Parses the input of the whole RotationalCluster in format produced by the
// << operator on PHV::RotationalCluster (can be found in phv/utils/utils.cpp)
PHV::RotationalCluster *SuperClusterBuilder::analyze_rotational_cluster(std::istream &scs) {
    std::string rot_clust("");
    ordered_set<PHV::AlignedCluster *> clusters;

    // The whole cluster is on one line
    std::getline(scs, rot_clust);

    // Remove initial whitespaces
    unsigned int i = 0;
    while (rot_clust.length() > i && ::isspace(rot_clust[i])) i++;
    rot_clust = rot_clust.substr(i, rot_clust.length() - 1);

    // Skip empty lines
    if (rot_clust.empty()) return new PHV::RotationalCluster(clusters);

    // Check and remove the outer '[]'
    if (rot_clust.length() < 2 || rot_clust[0] != '[' || rot_clust[rot_clust.length() - 1] != ']') {
        print_error("analyze_rotational_cluster() Failed - outer \'[]\' not found");
        return nullptr;
    }
    rot_clust = rot_clust.substr(1, rot_clust.length() - 2);

    unsigned int al_clust_from = 0;
    unsigned int al_clust_to = 0;
    unsigned int iter = 0;
    while (al_clust_to != rot_clust.length() - 1) {
        if (get_whole_parentheses(rot_clust, iter, &al_clust_from, &al_clust_to)) {
            print_error("analyze_rotational_cluster() Failed while extracting Aligned Cluster");
            return nullptr;
        }
        // Cut out the substring, without the outer '[]'
        std::string al_cluster_str =
            rot_clust.substr(al_clust_from + 1, al_clust_to - al_clust_from - 1);
        PHV::AlignedCluster *al_cluster = analyze_aligned_cluster(al_cluster_str);
        if (al_cluster == nullptr) {
            print_error("analyze_rotational_cluster() Failed while extracting Aligned Cluster");
            return nullptr;
        }
        // Add the aligned cluster
        clusters.push_back(al_cluster);
        // Set up for next one
        iter = al_clust_to + 1;
    }

    return new PHV::RotationalCluster(clusters);
}

// Parses the input of the whole AlignedCluster in format produced by the
// << operator on PHV::AlignedCluster (can be found in phv/utils/utils.cpp)
PHV::AlignedCluster *SuperClusterBuilder::analyze_aligned_cluster(std::string &str) {
    std::string slice_str;
    PHV::FieldSlice *slice;

    // Create slices list
    ordered_set<PHV::FieldSlice> slices;
    // Create string stream
    std::istringstream ss(str);

    // Get one slice seperated by the comma
    while (std::getline(ss, slice_str, ',')) {
        slice = analyze_field_slice(slice_str);
        if (slice == nullptr) {
            print_error("analyze_aligned_cluster() Failed while extracting Field Slice");
            return nullptr;
        }
        slices.push_back(*slice);
    }

    return new PHV::AlignedCluster(PHV::Kind::normal, slices);
}

// Parses the input of a single FieldSlice in format produced byt the
// << operator on PHV::FieldSlice (can be found in phv/phv_fields.cpp)
PHV::FieldSlice *SuperClusterBuilder::analyze_field_slice(std::string &str) {
    std::string t;
    unsigned align;
    bool was_align = false;
    unsigned vr_lo;
    unsigned vr_hi;
    bool was_vr = false;
    bool new_field = true;
    PHV::Field *f;

    // Remove whitespaces to get just the relevant str data for the key
    std::string slice_map_key = str;
    slice_map_key.erase(remove_if(slice_map_key.begin(), slice_map_key.end(), ::isspace),
                        slice_map_key.end());
    // Check if this slice already exists
    auto const find_slice = str_to_slice_m.find(slice_map_key);
    // Found - just return it
    if (find_slice != str_to_slice_m.end()) return find_slice->second;

    // At this point the slice does not yet exist => analyze and create it
    // Create string stream
    std::istringstream ss(str);

    // Start analysis
    t = get_token(ss);
    // Check if this field already exists
    auto const fm = str_to_field_m.find(t);
    // Found - work with this one
    if (fm != str_to_field_m.end()) {
        f = fm->second;
        new_field = false;
        // Otherwise init new one
    } else {
        f = new PHV::Field;
        f->metadata = false;
        f->updateValidContainerRange(ZeroToMax());
        std::istringstream namestr(t);
        // Get the name
        std::string name;
        std::getline(namestr, name, '<');
        f->name = cstring(name);
        // Get the width
        namestr >> f->size;
        str_to_field_m.insert(std::pair<const std::string, PHV::Field *>(t, f));
        new_field = true;
    }

    t = get_token(ss);
    if (t.length() >= 2 && t[0] == '^' && t[1] != 'b') {
        if (new_field) {
            // Remove '^'
            t = t.substr(1, t.length() - 1);
            std::istringstream alignstr(t);
            alignstr >> align;
            was_align = true;
        }
        t = get_token(ss);
    }
    if (t.length() >= 2 && t[0] == '^' && t[1] == 'b') {
        if (new_field) {
            // Remove '^bit['
            t = t.substr(5, t.length() - 5);
            std::istringstream vrstr(t);
            vrstr >> vr_lo;
            // Skip '..'
            vrstr.seekg(2, vrstr.cur);
            vrstr >> vr_hi;
            was_vr = true;
        }
        t = get_token(ss);
    }
    if (t.compare("bridge") == 0) {
        if (new_field) {
            f->bridged = true;
        }
        t = get_token(ss);
    }
    if (t.compare("meta") == 0) {
        if (new_field) {
            f->metadata = true;
        }
        t = get_token(ss);
    }
    if (t.compare("intrinsic") == 0) {
        if (new_field) {
            f->set_intrinsic(true);
        }
        t = get_token(ss);
    }
    if (t.length() >= 2 && t[0] == 'm' && t[1] != 'i') {
        if (new_field) {
            // TODO: mirror rozobrat
        }
        t = get_token(ss);
    }
    if (t.compare("pov") == 0) {
        if (new_field) {
            f->pov = true;
        }
        t = get_token(ss);
    }
    if (t.compare("deparsed-zero") == 0) {
        if (new_field) {
            f->set_deparser_zero_candidate(true);
        }
        t = get_token(ss);
    } else if (t.compare("deparsed") == 0) {
        if (new_field) {
            f->set_deparsed(true);
        }
        t = get_token(ss);
    }
    if (t.compare("solitary") == 0) {
        if (new_field) {
            f->set_solitary(0);
        }
        t = get_token(ss);
    }
    if (t.compare("no_split") == 0) {
        if (new_field) {
            f->set_no_split(true);
        }
        t = get_token(ss);
    }
    if (t.compare("deparsed_bottom_bits") == 0) {
        if (new_field) {
            f->set_deparsed_bottom_bits(true);
        }
        t = get_token(ss);
    }
    if (t.compare("deparsed_to_tm") == 0) {
        if (new_field) {
            f->set_deparsed_to_tm(true);
        }
        t = get_token(ss);
    }
    if (t.compare("exact_containers") == 0) {
        if (new_field) {
            f->set_exact_containers(true);
        }
        t = get_token(ss);
    }
    if (t.compare("wide_arith") == 0) {
        if (new_field) {
            f->add_wide_arith_start_bit(0);
        }
        t = get_token(ss);
    }
    if (t.compare("mocha") == 0) {
        if (new_field) {
            f->set_mocha_candidate(true);
        }
        t = get_token(ss);
    }
    if (t.compare("dark") == 0) {
        if (new_field) {
            f->set_dark_candidate(true);
        }
        t = get_token(ss);
    }

    // Remove '[]'
    if (t.length() >= 2 && t[0] == '[' && t[t.length() - 1] == ']') {
        t = t.substr(1, t.length() - 2);
    } else {
        print_error("analyze_field_slice() Slice range not defined properly");
        return nullptr;
    }
    std::istringstream ts(t);
    unsigned lo;
    unsigned hi;
    // Get lower bound
    ts >> lo;
    // Move over ':'
    ts.seekg(1, ts.cur);
    // Get upper bound
    ts >> hi;
    ClosedRange<RangeUnit::Bit, Endian::Little> br(lo, hi);

    // Also at this point update the alignment
    if (was_align) {
        // We need to decrement it by the lower bound of FieldSlice range
        // This means it has to be higher, while still retaining its
        // modulo 8 equivalence class
        if ((unsigned)br.lo > align) {
            align += 8 * ((br.lo - align) / 8);
            if ((br.lo - align) % 8) align += 8;
        }
        // Decrease by lower bound
        align -= br.lo;
        ClosedRange<RangeUnit::Bit, Endian::Little> align_range(align, align);
        FieldAlignment al(align_range);
        f->alignment = al;
    }
    // Also update the validContainerRange
    if (was_vr) {
        // Increase the size of valid range by the lower bound of slice range
        vr_hi += br.lo;
        ClosedRange<RangeUnit::Bit, Endian::Network> valid_range(vr_lo, vr_hi);
        f->updateValidContainerRange(valid_range);
    }

    // Create the slice
    PHV::FieldSlice *slice = new PHV::FieldSlice(f, br);
    // Add it to the map of slices
    str_to_slice_m.insert(std::pair<const std::string, PHV::FieldSlice *>(slice_map_key, slice));
    // And return it
    return slice;
}
