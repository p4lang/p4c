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

#ifndef EXTENSIONS_BF_P4C_COMMON_PARSE_ANNOTATIONS_H_
#define EXTENSIONS_BF_P4C_COMMON_PARSE_ANNOTATIONS_H_

#include "ir/ir.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "frontends/p4/parseAnnotations.h"

namespace BFN {

// wrapper for PARSE, PARSE_PAIR, and PARSE_TRIPLE
#define BFN_PARSE(pragmaClass, parse, tname, internal) {                                     \
    auto p = new Pragma(pragmaClass::name, pragmaClass::description, pragmaClass::help);     \
    std::pair<std::string, P4::ParseAnnotations::Handler> h = parse(pragmaClass::name, tname);   \
    addHandler(h.first, h.second);                                                           \
    Pragma::registerPragma(p, internal);                                                     \
    }

// wrapper for PARSE_EMPTY, PARSE_CONSTANT_OR_STRING_LITERAL, PARSE_EXPRESSION_LIST,
// PARSE_CONSTANT_LIST, PARSE_CONSTANT_OR_STRING_LITERAL_LIST, PARSE_STRING_LITERAL_LIST and
// PARSE_SKIP
#define BFN_PARSE_EMPTY(pragmaClass, parse, internal) {                                      \
    auto p = new Pragma(pragmaClass::name, pragmaClass::description, pragmaClass::help);     \
    std::pair<std::string, P4::ParseAnnotations::Handler> h = parse(pragmaClass::name);          \
    addHandler(h.first, h.second);                                                           \
    Pragma::registerPragma(p, internal);                                                     \
    }


/** Parses Barefoot-specific annotations. */
class ParseAnnotations : public P4::ParseAnnotations {
 public:
    ParseAnnotations() : P4::ParseAnnotations("BFN"_cs, true, {
                // Ignore p4v annotations.
                PARSE_SKIP("assert"_cs),
                PARSE_SKIP("assume"_cs),

                // Ignore p4runtime annotations
                PARSE_SKIP("brief"_cs),
                PARSE_SKIP("description"_cs),

                // Ignore unused annotations appearing in headers for v1model.
                PARSE_SKIP("metadata"_cs),
                PARSE_SKIP("alias"_cs),
                PARSE_SKIP("pipeline"_cs),
                PARSE_SKIP("deparser"_cs),
            }, true) {
        constexpr bool extPragma = false;  // externally supported
        constexpr bool intPragma = true;   // Barefoot internal
        BFN_PARSE(PragmaAdjustByteCount, PARSE, Expression, intPragma);
        BFN_PARSE(PragmaAlpm, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaAlpmPartitions, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaAlpmSubtreePartitions, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaAtcamPartitions, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaAtcamPartitionIndex, PARSE, StringLiteral, extPragma);
        BFN_PARSE_EMPTY(PragmaAlpmAtcamExcludeFieldMsbs, PARSE_EXPRESSION_LIST, extPragma);
        BFN_PARSE(PragmaCalculatedFieldUpdateLocation, PARSE, StringLiteral, extPragma);
        BFN_PARSE_EMPTY(PragmaBackwardCompatible, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaChainAddress, PARSE_EMPTY, intPragma);
        BFN_PARSE(PragmaChainTotalSize, PARSE, Expression, intPragma);
        BFN_PARSE_EMPTY(PragmaCommandLine, PARSE_CONSTANT_OR_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaCritical, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaDefaultPortmap, PARSE_EXPRESSION_LIST, intPragma);
        BFN_PARSE(PragmaDisableAtomicModify, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaDisableI2EReservedDropImplementation, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaDisableEgressMirrorIOSelectInitialization, PARSE_EMPTY, extPragma);
        BFN_PARSE(PragmaDoNotBridge, PARSE_PAIR, StringLiteral, extPragma);
        BFN_PARSE_EMPTY(PragmaDoNotUseClot, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE(PragmaDontTranslateExternMethod, PARSE, StringLiteral, intPragma);
        BFN_PARSE(PragmaDontMerge, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaDontUnroll, PARSE_EMPTY, extPragma);
        BFN_PARSE(PragmaDynamicTableKeyMasks, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaEgressIntrinsicMetadataOpt, PARSE_EMPTY, extPragma);
        BFN_PARSE(PragmaEntriesWithRanges, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaFieldListFieldSlice, PARSE_TRIPLE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaFlexible, PARSE_EMPTY, extPragma);
        BFN_PARSE(PragmaForceImmediate, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaForceShift, PARSE_PAIR, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaGFMParityEnable, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaGhostMetadata, PARSE_EMPTY, intPragma);
        BFN_PARSE(PragmaIdletimeInterval, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaIdletimePerFlow, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaIdletimePrecision, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaIdletimeTwoWayNotification, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaIgnoreTableDependency, PARSE, StringLiteral, extPragma);
        BFN_PARSE(PragmaImmediate, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaIxbarGroupNum, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaIRContextBasedDebugLogging, PARSE_EMPTY, intPragma);
        BFN_PARSE_EMPTY(PragmaInHash, PARSE_EMPTY, intPragma);
        BFN_PARSE_EMPTY(PragmaIntrinsicMetadata, PARSE_EMPTY, intPragma);
        BFN_PARSE(PragmaLrtEnable, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaLrtScale, PARSE, Expression, intPragma);
        BFN_PARSE(PragmaMaxActions, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaMaxLoopDepth, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaMinWidth, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaMode, PARSE, StringLiteral, intPragma);
        BFN_PARSE_EMPTY(PragmaNoFieldInits, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaNoGatewayConversion, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaNotExtractedInEgress, PARSE_EMPTY, intPragma);
        BFN_PARSE(PragmaOverridePhase0ActionName, PARSE, StringLiteral, extPragma);
        BFN_PARSE(PragmaOverridePhase0TableName, PARSE, StringLiteral, extPragma);
        BFN_PARSE(PragmaPack, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaPadding, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaPacketEntry, PARSE_EMPTY, extPragma);
        BFN_PARSE(PragmaPhase0, PARSE_TRIPLE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaPlacementPriority, PARSE_EXPRESSION_LIST, extPragma);
        BFN_PARSE(PragmaPreColor, PARSE, Expression, intPragma);
        BFN_PARSE(PragmaProxyHashAlgorithm, PARSE, StringLiteral, extPragma);
        BFN_PARSE(PragmaProxyHashWidth, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaPhvLimit, PARSE_SKIP, intPragma);
        BFN_PARSE(PragmaRandomSeed, PARSE, Expression, intPragma);
        BFN_PARSE(PragmaReductionOrGroup, PARSE, StringLiteral, intPragma);
        BFN_PARSE(PragmaReg, PARSE, StringLiteral, intPragma);
        BFN_PARSE(PragmaResidualChecksumParserUpdateLocation, PARSE, StringLiteral, intPragma);
        BFN_PARSE(PragmaSelectorMaxGroupSize, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaSelectorNumMaxGroups, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaSelectorEnableScramble, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaSeparateGateway, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaStage, PARSE_EXPRESSION_LIST, extPragma);
        BFN_PARSE(PragmaSymmetric, PARSE_PAIR, Expression, extPragma);
        BFN_PARSE(PragmaTerminateParsing, PARSE, StringLiteral, extPragma);
        BFN_PARSE(PragmaTernary, PARSE, Expression, intPragma);  // unlikely to need it
        BFN_PARSE(PragmaUseHashAction, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaUserAnnotation, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE(PragmaWays, PARSE, Expression, extPragma);
        BFN_PARSE(PragmaHashMask, PARSE, Expression, intPragma);

        // Meter pragmas
        BFN_PARSE(PragmaRed, PARSE, Expression, intPragma);
        BFN_PARSE(PragmaYellow, PARSE, Expression, intPragma);
        BFN_PARSE(PragmaGreen, PARSE, Expression, intPragma);
        BFN_PARSE(PragmaMeterProfile, PARSE, Expression, extPragma);
        BFN_PARSE_EMPTY(PragmaTrueEgressAccounting,  PARSE_EMPTY, extPragma);

        BFN_PARSE_EMPTY(PragmaHeaderChecksum,  PARSE_EMPTY, intPragma);
        BFN_PARSE_EMPTY(PragmaPayloadChecksum, PARSE_EMPTY, intPragma);

        // pa_ pragmas
        BFN_PARSE_EMPTY(PragmaAlias, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaAtomic, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaAutoInitMetadata, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaBytePack, PARSE_EXPRESSION_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaContainerSize, PARSE_EXPRESSION_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaContainerType, PARSE_STRING_LITERAL_LIST, extPragma);
        // FIXME: is DisableDeparseZero internal?
        BFN_PARSE_EMPTY(PragmaDisableDeparseZero, PARSE_STRING_LITERAL_LIST, intPragma);
        BFN_PARSE_EMPTY(PragmaMutuallyExclusive, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaNoInit, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaNoOverlay, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaNoPack, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaNotDeparsed, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaNotParsed, PARSE_STRING_LITERAL_LIST, extPragma);
        BFN_PARSE_EMPTY(PragmaParserBandwidthOpt, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaParserGroupMonogress, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaAllowPOVonMocha, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaPrioritizeARAinits, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaQuickPhvAlloc, PARSE_EMPTY, extPragma);
        BFN_PARSE_EMPTY(PragmaSolitary, PARSE_STRING_LITERAL_LIST, extPragma);

        // internal annotation to be removed
        BFN_PARSE(PragmaActionSelectorHashFieldCalcName, PARSE, StringLiteral, intPragma);
        BFN_PARSE(PragmaActionSelectorHashFieldCalcOutputWidth, PARSE, Expression, intPragma);
        BFN_PARSE(PragmaActionSelectorHashFieldListName, PARSE, StringLiteral, intPragma);
        BFN_PARSE(PragmaAlgorithm, PARSE, StringLiteral, intPragma);
    }
};

}  // namespace BFN

#endif /* EXTENSIONS_BF_P4C_COMMON_PARSE_ANNOTATIONS_H_ */
