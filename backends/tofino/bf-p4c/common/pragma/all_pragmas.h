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

#ifndef EXTENSIONS_BF_P4C_COMMON_PRAGMA_ALL_PRAGMAS_H_
#define EXTENSIONS_BF_P4C_COMMON_PRAGMA_ALL_PRAGMAS_H_

#include "bf-p4c/common/pragma/pragma.h"
#include "bf-p4c/parde/clot/pragma/do_not_use_clot.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"

#define DEFINE_PRAGMA_CLASS(cname)      \
  class cname {                         \
   public:                              \
     static const char *name;           \
     static const char *description;    \
     static const char *help;           \
  }

DEFINE_PRAGMA_CLASS(PragmaAdjustByteCount);
DEFINE_PRAGMA_CLASS(PragmaAlpm);
DEFINE_PRAGMA_CLASS(PragmaAlpmPartitions);
DEFINE_PRAGMA_CLASS(PragmaAlpmSubtreePartitions);
DEFINE_PRAGMA_CLASS(PragmaAtcamPartitions);
DEFINE_PRAGMA_CLASS(PragmaAtcamPartitionIndex);
DEFINE_PRAGMA_CLASS(PragmaAlpmAtcamExcludeFieldMsbs);
DEFINE_PRAGMA_CLASS(PragmaAutoInitMetadata);
DEFINE_PRAGMA_CLASS(PragmaQuickPhvAlloc);
DEFINE_PRAGMA_CLASS(PragmaParserBandwidthOpt);
DEFINE_PRAGMA_CLASS(PragmaParserGroupMonogress);
DEFINE_PRAGMA_CLASS(PragmaAllowPOVonMocha);
DEFINE_PRAGMA_CLASS(PragmaPrioritizeARAinits);
DEFINE_PRAGMA_CLASS(PragmaCalculatedFieldUpdateLocation);
DEFINE_PRAGMA_CLASS(PragmaChainAddress);
DEFINE_PRAGMA_CLASS(PragmaChainTotalSize);
DEFINE_PRAGMA_CLASS(PragmaCommandLine);
DEFINE_PRAGMA_CLASS(PragmaCritical);
DEFINE_PRAGMA_CLASS(PragmaDefaultPortmap);
DEFINE_PRAGMA_CLASS(PragmaDisableAtomicModify);
DEFINE_PRAGMA_CLASS(PragmaDisableI2EReservedDropImplementation);
DEFINE_PRAGMA_CLASS(PragmaDisableEgressMirrorIOSelectInitialization);
DEFINE_PRAGMA_CLASS(PragmaDontTranslateExternMethod);
DEFINE_PRAGMA_CLASS(PragmaDoNotBridge);
DEFINE_PRAGMA_CLASS(PragmaDynamicTableKeyMasks);
DEFINE_PRAGMA_CLASS(PragmaEntriesWithRanges);
DEFINE_PRAGMA_CLASS(PragmaEgressIntrinsicMetadataOpt);
DEFINE_PRAGMA_CLASS(PragmaFieldListFieldSlice);
DEFINE_PRAGMA_CLASS(PragmaFlexible);
DEFINE_PRAGMA_CLASS(PragmaForceImmediate);
DEFINE_PRAGMA_CLASS(PragmaForceShift);
DEFINE_PRAGMA_CLASS(PragmaGFMParityEnable);
DEFINE_PRAGMA_CLASS(PragmaGhostMetadata);
DEFINE_PRAGMA_CLASS(PragmaIdletimeInterval);
DEFINE_PRAGMA_CLASS(PragmaIdletimePerFlow);
DEFINE_PRAGMA_CLASS(PragmaIdletimePrecision);
DEFINE_PRAGMA_CLASS(PragmaIdletimeTwoWayNotification);
DEFINE_PRAGMA_CLASS(PragmaImmediate);
DEFINE_PRAGMA_CLASS(PragmaInHash);
DEFINE_PRAGMA_CLASS(PragmaIntrinsicMetadata);
DEFINE_PRAGMA_CLASS(PragmaIRContextBasedDebugLogging);
DEFINE_PRAGMA_CLASS(PragmaIgnoreTableDependency);
DEFINE_PRAGMA_CLASS(PragmaIxbarGroupNum);
DEFINE_PRAGMA_CLASS(PragmaLrtEnable);
DEFINE_PRAGMA_CLASS(PragmaLrtScale);
DEFINE_PRAGMA_CLASS(PragmaMaxActions);
DEFINE_PRAGMA_CLASS(PragmaMaxLoopDepth);
DEFINE_PRAGMA_CLASS(PragmaMinWidth);
DEFINE_PRAGMA_CLASS(PragmaMode);
DEFINE_PRAGMA_CLASS(PragmaDontMerge);
DEFINE_PRAGMA_CLASS(PragmaNotExtractedInEgress);
DEFINE_PRAGMA_CLASS(PragmaNoGatewayConversion);
DEFINE_PRAGMA_CLASS(PragmaNoVersioning);
DEFINE_PRAGMA_CLASS(PragmaPack);
DEFINE_PRAGMA_CLASS(PragmaPadding);
DEFINE_PRAGMA_CLASS(PragmaPacketEntry);
DEFINE_PRAGMA_CLASS(PragmaPhase0);
DEFINE_PRAGMA_CLASS(PragmaPhvLimit);
DEFINE_PRAGMA_CLASS(PragmaOverridePhase0TableName);
DEFINE_PRAGMA_CLASS(PragmaOverridePhase0ActionName);
DEFINE_PRAGMA_CLASS(PragmaPlacementPriority);
DEFINE_PRAGMA_CLASS(PragmaPreColor);
DEFINE_PRAGMA_CLASS(PragmaProxyHashAlgorithm);
DEFINE_PRAGMA_CLASS(PragmaProxyHashWidth);
DEFINE_PRAGMA_CLASS(PragmaRandomSeed);
DEFINE_PRAGMA_CLASS(PragmaReductionOrGroup);
DEFINE_PRAGMA_CLASS(PragmaReg);
DEFINE_PRAGMA_CLASS(PragmaResidualChecksumParserUpdateLocation);
DEFINE_PRAGMA_CLASS(PragmaSelectorMaxGroupSize);
DEFINE_PRAGMA_CLASS(PragmaSelectorNumMaxGroups);
DEFINE_PRAGMA_CLASS(PragmaSelectorEnableScramble);
DEFINE_PRAGMA_CLASS(PragmaSymmetric);
DEFINE_PRAGMA_CLASS(PragmaStage);
DEFINE_PRAGMA_CLASS(PragmaTerminateParsing);
DEFINE_PRAGMA_CLASS(PragmaDontUnroll);
DEFINE_PRAGMA_CLASS(PragmaTernary);
DEFINE_PRAGMA_CLASS(PragmaUseHashAction);
DEFINE_PRAGMA_CLASS(PragmaUserAnnotation);
DEFINE_PRAGMA_CLASS(PragmaWays);
DEFINE_PRAGMA_CLASS(PragmaNoFieldInits);
DEFINE_PRAGMA_CLASS(PragmaSeparateGateway);
DEFINE_PRAGMA_CLASS(PragmaBackwardCompatible);
DEFINE_PRAGMA_CLASS(PragmaHashMask);
// meter
DEFINE_PRAGMA_CLASS(PragmaRed);
DEFINE_PRAGMA_CLASS(PragmaYellow);
DEFINE_PRAGMA_CLASS(PragmaGreen);
DEFINE_PRAGMA_CLASS(PragmaMeterProfile);
DEFINE_PRAGMA_CLASS(PragmaTrueEgressAccounting);
// p4testgen
DEFINE_PRAGMA_CLASS(PragmaHeaderChecksum);
DEFINE_PRAGMA_CLASS(PragmaPayloadChecksum);

DEFINE_PRAGMA_CLASS(PragmaNotParsed);
DEFINE_PRAGMA_CLASS(PragmaNotDeparsed);
DEFINE_PRAGMA_CLASS(PragmaDisableDeparseZero);

// internal annotations to be removed
DEFINE_PRAGMA_CLASS(PragmaActionSelectorHashFieldCalcName);
DEFINE_PRAGMA_CLASS(PragmaActionSelectorHashFieldListName);
DEFINE_PRAGMA_CLASS(PragmaAlgorithm);
DEFINE_PRAGMA_CLASS(PragmaActionSelectorHashFieldCalcOutputWidth);

/*
DEFINE_PRAGMA_CLASS(Pragma);
*/

#endif  /* EXTENSIONS_BF_P4C_COMMON_PRAGMA_ALL_PRAGMAS_H_ */
