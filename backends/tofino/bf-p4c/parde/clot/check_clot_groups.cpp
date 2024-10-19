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

#include "check_clot_groups.h"

bool CheckClotGroups::preorder(const IR::BFN::AbstractDeparser *dp) {
    if (LOGGING(5)) dump(::Log::Detail::fileLogOutput(__FILE__), dp);
    field_dictionary.clear();
    return true;
}

bool CheckClotGroups::preorder(const IR::BFN::EmitField *ef) {
    const PHV::Field *field = phv.field(ef->source->field);
    bool newspan = false;
    if (auto *cl = clot.whole_field_clot(field)) {
        LOG2("clot " << cl->tag << ": " << field->size << " bits from " << field->name);
        if (field_dictionary.empty() || field_dictionary.back().clot != cl) {
            field_dictionary.emplace_back(cl);
            newspan = true;
        }
    } else {
        LOG2("nonclot: " << field->size << " bits from " << field->name);
        if (field_dictionary.empty() || field_dictionary.back().clot != nullptr) {
            field_dictionary.emplace_back(fde_span());
            newspan = true;
        }
    }
    fde_span &span = field_dictionary.back();
    span.size += field->size;
    if (newspan && field_dictionary.size() > 1) {
        fde_span &prev = field_dictionary.at(field_dictionary.size() - 2);
        if (prev.size % 8U != 0) {
            /* boundary between spans not on a byte boundary! */
            if (!span.clot) {
                /* non-clot after clot, so steal bits to move forward */
                unsigned adj = 8 - (prev.size % 8U);
                LOG2("boundary from clot to non-clot not byte aligned -- move +" << adj << " bits");
                prev.size += adj;
                span.size -= adj;
            } else if (!prev.clot) {
                /* clot after non-clot, move back */
                unsigned adj = prev.size % 8U;
                LOG2("boundary from non-clot to clot not byte aligned -- move -" << adj << " bits");
                prev.size -= adj;
                span.size += adj;
            } else {
                BUG("adjacent clots not split on byte boundary");
            }
        }
    }
    return false;
}

void CheckClotGroups::postorder(const IR::BFN::AbstractDeparser *dp) {
    unsigned next_chunk = 0;
    unsigned current_group = 0, clots_in_group = 0;
    unsigned pad_chunks = 0, pad_bytes = 0;
    for (auto &span : field_dictionary) {
        BUG_CHECK(span.size % 8U == 0, "fde chunk not byte aligned");
        if (next_chunk / DEPARSER_CHUNKS_PER_GROUP != current_group) {
            current_group = next_chunk / DEPARSER_CHUNKS_PER_GROUP;
            clots_in_group = 0;
        }
        span.chunk = next_chunk;
        unsigned end = next_chunk + (span.size - 1) / (8 * DEPARSER_CHUNK_SIZE);
        if (unsigned tail = (span.size / 8U) % DEPARSER_CHUNK_SIZE)
            pad_bytes += DEPARSER_CHUNK_SIZE - tail;
        if (span.clot) {
            if (++clots_in_group > DEPARSER_CHUNKS_PER_GROUP ||
                end / DEPARSER_CHUNKS_PER_GROUP != span.chunk / DEPARSER_CHUNKS_PER_GROUP) {
                unsigned gap = DEPARSER_CHUNKS_PER_GROUP - span.chunk % DEPARSER_CHUNKS_PER_GROUP;
                LOG2("insert gap of "
                     << gap << " chunks before clot " << span.clot->tag
                     << (clots_in_group > DEPARSER_CHUNKS_PER_GROUP ? " (too many clots in group)"
                                                                    : " (clot group alignment)"));
                span.chunk += gap;
                end += gap;
                pad_chunks += gap;
            }
        }
        next_chunk = end + 1;
    }
    LOG1(dp->gress << " field dictionary with " << next_chunk << " total chunks skipped "
                   << pad_bytes << " bytes and " << pad_chunks << " whole chunks for padding");
    if (next_chunk > DEPARSER_CHUNK_GROUPS * DEPARSER_CHUNKS_PER_GROUP) {
        error(ErrorType::ERR_OVERLIMIT,
              "%sToo many field dictionary chunks (%d) required in "
              "%s deparser, need to have fewer or smaller headers",
              dp->srcInfo, next_chunk, toString(dp->gress));
    }
}
