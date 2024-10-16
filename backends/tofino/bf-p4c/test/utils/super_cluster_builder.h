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

#ifndef EXTENSIONS_BF_P4C_TEST_UTILS_SUPER_CLUSTER_BUILDER_H_
#define EXTENSIONS_BF_P4C_TEST_UTILS_SUPER_CLUSTER_BUILDER_H_

#include <cctype>
#include <iostream>
#include <algorithm>
#include <string>
#include <istream>
#include <sstream>
#include <optional>

#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/allocate_phv.h"
#include "bf-p4c/ir/bitrange.h"

/// A helpfull class for building SuperClusters
class SuperClusterBuilder {
 public:
    /// Constructor
    SuperClusterBuilder() {}

    /// Main function that should be called to return the built SC
    /// @param scs Stream with the text represantation of the SuperCluster
    /// @returns Pointer to a SuperCluster or NULL in case of an error
    // The format of the file is as follows:
    // SUPERCLUSTER Uid: <uid>\n
    // <slice lists>\n
    // <rotational clusters>\n
    //
    // <uid> ::= <number>
    // <slice lists> ::= slice lists: <lists of slices>
    // <lists of slices> ::=
    // <lists of slices> ::= [ <slices> ]\n<list of slices>
    // <slices> ::=
    // <slices> ::= <slice>\n<slices>
    // <rotational clusters> ::= rotational clusters: { }
    // <rotational clusters> ::= rotational clusters:\n<list of rotational clusters>
    // <list of rotational clusters> ::=
    // <list of rotational clusters> ::= <rotational cluster>\n<list of rotational clusters>
    // <rotational cluster> ::= [<aligned cluster><aligned cluster continued>]
    // <aligned cluster continued> ::=
    // <aligned cluster continued> ::= , <aligned cluster><aligned cluster continued>
    // <aligned cluster> ::= [<slice><slice continued>]
    // <slice continued> ::=
    // <slice continued> ::= , <slice><slice continued>
    // <slice> ::= <field> <optional field attributes> <field range>
    // <field> ::= <identifier>'<'<number>'>'
    // <field range> ::= [<number>:<number>]
    // For possible optional field attributes see phv/phv_fields.cpp
    //   method std::ostream &operator<<(std::ostream &out, const PHV::FieldSlice& fs)
    std::optional<PHV::SuperCluster*> build_super_cluster(std::istream &scs);

 private:
    /// Map of fields
    /// key is the name<width> of the field
    /// value is the pointer to the PHV::Field object
    ordered_map<const std::string, PHV::Field*> str_to_field_m;
    /// Map of slices
    /// key is the string representation of the slice
    /// value is the pointer to the PHV::FieldSlice object
    ordered_map<const std::string, PHV::FieldSlice*> str_to_slice_m;

    /// Private functions that analyze different parts of the SuperCluster
    /// They work with the 'scs_m'

    /// Analysis of the whole SuperCluster
    /// @param scs Stream representation of the SuperCluster
    /// @returns NULL in case of error, extracted SuperCluster otherwise
    std::optional<PHV::SuperCluster*> analyze_super_cluster(std::istream &scs);
    /// Analysis of a single SliceList
    /// @param scs Stream representation of the SuperCluster
    /// @returns NULL in case of error, extracted FieldSlice otherwise
    PHV::SuperCluster::SliceList* analyze_slice_list(std::istream &scs);
    /// Analysis of a single RotationalCluster
    /// @param scs Stream representation of the SuperCluster
    /// @returns NULL in case of error, extracted RotationalCluster otherwise
    PHV::RotationalCluster* analyze_rotational_cluster(std::istream &scs);
    /// Analysis of AlignedCluster
    /// @param str String representation of the FieldSlice
    /// @returns NULL in case of error, extracted AlignedCluster otherwise
    PHV::AlignedCluster* analyze_aligned_cluster(std::string &str);
    /// Analysis of a single FieldSlice
    /// @param str String representation of the FieldSlice
    /// @returns NULL in case of error, extracted FieldSlice otherwise
    PHV::FieldSlice* analyze_field_slice(std::string &str);
};

#endif /* EXTENSIONS_BF_P4C_TEST_UTILS_SUPER_CLUSTER_BUILDER_H_ */
