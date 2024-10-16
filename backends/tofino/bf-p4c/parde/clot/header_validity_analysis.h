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

#ifndef EXTENSIONS_BF_P4C_PARDE_CLOT_HEADER_VALIDITY_ANALYSIS_H_
#define EXTENSIONS_BF_P4C_PARDE_CLOT_HEADER_VALIDITY_ANALYSIS_H_

#include "field_slice_set.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv_fields.h"

/// @brief Identify headers marked as valid/invalid in MAU pipeline and find correlations between
/// removal of headers of interest.
///
/// Finds correlations between the removal of headers in the MAU pipeline. In the back end, header
/// removal is represented by the clearing of POV bits, so this analysis really looks at the
/// correlation between when POV bits are cleared.
///
/// The input of this analysis is a set expressing the correlations that the client of the analysis
/// is interested in. This is done to avoid any unnecessary exponential blow-up in the size of the
/// result: the input acts as a filter on the results produced.
///
/// As a running example, consider a pipeline whose behaviour looks like:
///
///   if (foo) {
///     h1.setInvalid();
///     h2.setInvalid();
///   } else {
///     h1.setInvalid();
///     h3.setInvalid();
///   }
///
///   h3.setValid();
///
///   if (bar) h4.setInvalid();
///   if (baz) h5.setInvalid();
///
/// Let S1 be the set {{pov1, pov2, pov3, pov4, pov5}}. Given as input to the analysis, S1
/// indicates that the client is interested in all correlations between all headers.
///
/// Let S2 be the set {{pov1, pov2}, {pov2, pov3}, {pov2, pov4}}. Given as input, this set
/// indicates that we are only interested in pairwise correlations between h2 and h1, h3, and h4.
///
/// The output of this analysis is a map, wherein each set S in the input is mapped to subsets of S
/// that might be cleared. There may be multiple subsets when different paths through the pipeline
/// result in different POV bits being cleared.
///
/// If a header is removed and subsequently added, in the output of this analysis, it will still
/// appear as having been removed.
///
/// Given S1 as input to the analysis, the result would resemble
///
///   {pov1, po2, pov3, pov4, pov5}
///       ↦ {{pov1, pov2}, {pov1, pov2, pov4}, {pov1, pov2, pov5}, {pov1, pov2, pov4, pov5},
///          {pov1, pov3}, {pov1, pov3, pov4}, {pov1, pov3, pov5}, {pov1, pov3, pov4, pov5}}
///
/// We can expect this kind of result to be exponential in the number of headers.
///
/// Alternatively, if S2 were given as input, the result would be:
///
///   {pov1, pov2} ↦ {{pov1}, {pov1, pov2}}
///   {pov2, pov3} ↦ {{pov2}, {pov3}}
///   {pov2, pov4} ↦ {{}, {pov2}, {pov4}, {pov2, pov4}}
///
/// For clients only interested in pairwise correlations, we can expect the result to be quadratic
/// in the number of headers.
class HeaderValidityAnalysis : public MauInspector {
 public:
    using ResultMap = std::map<FieldSliceSet, std::set<FieldSliceSet>>;

    const PhvInfo& phvInfo;

    /// The correlations that the client is interested in, indexed by POV bit.
    std::map<const PHV::FieldSlice*,
             std::set<FieldSliceSet>,
             PHV::FieldSlice::Less>* interestedCorrelations;

    std::set<const PHV::Field*> povBitsSetInvalidInMau;
    std::set<const PHV::Field*> povBitsSetValidInMau;

    std::map<const PHV::Field*, std::set<const IR::MAU::Action*>> povBitsUpdateActions;
    std::map<const PHV::Field*, std::set<const IR::MAU::Action*>> povBitsUpdateOrInvalidateActions;
    std::map<const PHV::Field*, std::set<const IR::MAU::Action*>> povBitsUpdateOrValidateActions;
    std::map<const PHV::Field*, std::set<const IR::MAU::Action*>> povBitsInvalidateActions;
    std::map<const PHV::Field*, std::set<const IR::MAU::Action*>> povBitsValidateActions;

    SymBitMatrix povBitsAlwaysInvalidateTogether;
    SymBitMatrix povBitsAlwaysValidateTogether;

    /// The output of this analysis.
    ResultMap resultMap;

    HeaderValidityAnalysis(const PhvInfo& phvInfo, const std::set<FieldSliceSet>& correlations);
    HeaderValidityAnalysis(const HeaderValidityAnalysis&) = default;
    HeaderValidityAnalysis(HeaderValidityAnalysis&&) = default;

    Visitor::profile_t init_apply(const IR::Node* root) override;
    bool preorder(const IR::MAU::Instruction* instruction) override;
    bool preorder(const IR::MAU::Action *act) override;
    void end_apply() override;

    HeaderValidityAnalysis* clone() const override;
    HeaderValidityAnalysis& flow_clone() override;
    void flow_merge(Visitor& v) override;
    // FIXME -- not a ControlFlowVisitor, so there will never be any clones to merge...
    // void flow_copy(::ControlFlowVisitor& v) override;
};

#endif /* EXTENSIONS_BF_P4C_PARDE_CLOT_HEADER_VALIDITY_ANALYSIS_H_ */
