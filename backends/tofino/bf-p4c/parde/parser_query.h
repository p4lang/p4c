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

#ifndef BF_P4C_PARDE_PARSER_QUERY_H_
#define BF_P4C_PARDE_PARSER_QUERY_H_

#include <ir/ir.h>
#include "ir/pass_manager.h"

#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_info.h"

class PhvInfo;
class MapFieldToParserStates;

/// @brief Collection of functions to check parser information
///
/// These functions rely on data provided by other classes, hence the reason for encapsulating
/// inside a struct
struct ParserQuery {
    const CollectParserInfo& parser_info;
    const MapFieldToParserStates& field_to_states;

    /// Do two extracts use the same constant value?
    bool same_const_source(const IR::BFN::ParserPrimitive *pp,
                           const IR::BFN::ParserPrimitive *qp) const;

    /// Check if the effect of @p p is before the effect of @p q in the parser.
    /// @p writes is the set of writes that belong to the same field and both @p p and @p q must
    /// be in this set.
    /// @p ps must be state of @p p or nullptr. @p qs must be state of @p q or nullptr.
    bool is_before(const ordered_set<const IR::BFN::ParserPrimitive *> &writes,
                   const IR::BFN::Parser *parser,
                   const IR::BFN::ParserPrimitive *p,
                   const IR::BFN::ParserState *ps,
                   const IR::BFN::ParserPrimitive *q,
                   const IR::BFN::ParserState *qs = nullptr) const;

    /// Find all prior writes for a given write
    ///
    /// @param p      The write for which to find prior writes
    /// @param writes The set of writes from which to identify prior writes
    ///
    /// @returns An ordered_set of all writes that occur prior to @p
    ordered_set<const IR::BFN::ParserPrimitive*>
    get_previous_writes(const IR::BFN::ParserPrimitive* p,
                        const ordered_set<const IR::BFN::ParserPrimitive*>& writes) const;

    /// Are all writes in @p writes single-write (i.e., no path through the
    /// parse graph executes more than one write).
    bool is_single_write(const ordered_set<const IR::BFN::ParserPrimitive *> &writes) const;

    /// Find the first writes, i.e. inits, given the set of @p writes. The set of writes
    /// belong to the same field. An init is a write that has no prior write in the parser IR.
    ordered_set<const IR::BFN::ParserPrimitive*>
    find_inits(const ordered_set<const IR::BFN::ParserPrimitive*>& writes) const;

    /// Find the first writes, i.e. inits, corresponding to the container @p c.  An init is a write
    /// that has no prior write in the parser IR.
    ordered_set<const IR::BFN::ParserPrimitive*> find_inits(PHV::Container c) const;

    ParserQuery(const CollectParserInfo &pi, const MapFieldToParserStates &fs)
        : parser_info(pi), field_to_states(fs) {}
};

#endif /*BF_P4C_PARDE_PARSER_QUERY_H_*/

