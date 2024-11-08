/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef BACKENDS_BMV2_COMMON_ACTION_H_
#define BACKENDS_BMV2_COMMON_ACTION_H_

#include "helpers.h"
#include "ir/ir.h"

namespace P4::BMV2 {

/// JumpLabelInfo holds information about jump instruction labels
/// (i.e. destinations of jump primitives) during the construction of
/// the primitives for one action in the BMv2 JSON data.
struct JumpLabelInfo {
    // The number of jump labels that have been created so far, for
    // the current action where we are generating its primitives.
    // Jump labels are allocated starting from 0 and incrementing from
    // there.
    int numLabels;
    // The jump label that is positioned at the end of the action
    // body.  This is needed if we wish to implement a 'return' or
    // 'exit' statement within a conditionally executed part of an
    // action body, to jump to the end of the action.
    int labelIdEndOfAction;
    // Let F be the set of offsets in the action that contain a
    // primitive "_jump" or _jump_if_zero".  For each offset f in F,
    // offsetToTargetLabelId[f] is the label ID to which the primitive
    // should jump.
    std::map<unsigned int, unsigned int> offsetToTargetLabelId;
    // For each offset f in F, offsetToJumpParams[f] is the array
    // of parameters that should be used as operands to the primitive,
    // except for the offset, which will only be added at the end of
    // method convertActionBodyTop.
    std::map<unsigned int, Util::JsonArray *> offsetToJumpParams;
    // labelIdToJumpOffset[labelId] equal to offset means that the
    // position of that labelId within the array of primitives for an
    // action is 'offset'.  _jump and _jump_if_zero primitive actions
    // must use 'offset' as their offset parameter in order to jump to
    // that primitive.
    std::map<unsigned int, unsigned int> labelIdToJumpOffset;
};

class ActionConverter : public Inspector {
    ConversionContext *ctxt;

    void convertActionBodyTop(const IR::Vector<IR::StatOrDecl> *body, Util::JsonArray *result);
    void convertActionBody(const IR::Vector<IR::StatOrDecl> *body, Util::JsonArray *result,
                           bool inConditional, JumpLabelInfo *info);
    void convertActionParams(const IR::ParameterList *parameters, Util::JsonArray *params);
    cstring jsonAssignment(const IR::Type *type);
    void postorder(const IR::P4Action *action) override;

 public:
    const bool emitExterns;
    explicit ActionConverter(ConversionContext *ctxt, const bool &emitExterns_)
        : ctxt(ctxt), emitExterns(emitExterns_) {
        setName("ConvertActions");
    }
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_COMMON_ACTION_H_ */
