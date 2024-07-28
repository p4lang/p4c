/*
Copyright 2024 Marvell Technology, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef BACKENDS_BMV2_PORTABLE_COMMON_PORTABLE_H_
#define BACKENDS_BMV2_PORTABLE_COMMON_PORTABLE_H_

#include "backends/bmv2/common/action.h"
#include "backends/bmv2/common/control.h"
#include "backends/bmv2/common/deparser.h"
#include "backends/bmv2/common/extern.h"
#include "backends/bmv2/common/header.h"
#include "backends/bmv2/common/helpers.h"
#include "backends/bmv2/common/lower.h"
#include "backends/bmv2/common/parser.h"
#include "backends/common/portableProgramStructure.h"
#include "backends/common/programStructure.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"
#include "lib/json.h"

namespace BMV2 {

class PortableCodeGenerator {
 public:
    // PortableCodeGenerator() {}
    // : PortableProgramStructure(refMap, typeMap) {}

    unsigned error_width = 32;

    void createStructLike(ConversionContext *ctxt, const IR::Type_StructLike *st,
                          P4::PortableProgramStructure *structure);
    void createTypes(ConversionContext *ctxt, P4::PortableProgramStructure *structure);
    void createHeaders(ConversionContext *ctxt, P4::PortableProgramStructure *structure);
    void createScalars(ConversionContext *ctxt, P4::PortableProgramStructure *structure);
    void createExterns();
    void createActions(ConversionContext *ctxt, P4::PortableProgramStructure *structure);
    void createGlobals();
    cstring convertHashAlgorithm(cstring algo);
};

EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Checksum)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Counter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(DirectCounter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Meter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(DirectMeter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Random)
EXTERN_CONVERTER_W_INSTANCE(ActionProfile)
EXTERN_CONVERTER_W_INSTANCE(ActionSelector)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Digest)

}  // namespace BMV2

#endif /* BACKENDS_BMV2_PORTABLE_COMMON_PORTABLE_H_ */
