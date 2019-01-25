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

#ifndef BACKENDS_BMV2_COMMON_BACKEND_H_
#define BACKENDS_BMV2_COMMON_BACKEND_H_

#include "controlFlowGraph.h"
#include "expression.h"
#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/json.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "JsonObjects.h"
#include "metermap.h"
#include "midend/convertEnums.h"
#include "midend/actionSynthesis.h"
#include "midend/removeLeftSlices.h"
#include "sharedActionSelectorCheck.h"
#include "options.h"

namespace BMV2 {

enum gress_t { INGRESS, EGRESS };
enum block_t { PARSER, PIPELINE, DEPARSER, V1_PARSER, V1_DEPARSER,
               V1_INGRESS, V1_EGRESS, V1_VERIFY, V1_COMPUTE};

class ExpressionConverter;

// Backend is a the base class for SimpleSwitchBackend and PortableSwitchBackend.
class Backend {
 public:
    BMV2Options&                     options;
    P4::ReferenceMap*                refMap;
    P4::TypeMap*                     typeMap;
    P4::ConvertEnums::EnumMapping*   enumMap;
    P4::P4CoreLibrary&               corelib;
    BMV2::JsonObjects*               json;
    ExpressionConverter*             conv;
    const IR::ToplevelBlock*         toplevel;

 public:
    Backend(BMV2Options& options, P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
            P4::ConvertEnums::EnumMapping* enumMap) :
        options(options),
        refMap(refMap), typeMap(typeMap), enumMap(enumMap),
        corelib(P4::P4CoreLibrary::instance), json(new BMV2::JsonObjects()) {
        refMap->setIsV1(options.isv1());
        }
    void serialize(std::ostream& out) const { json->toplevel->serialize(out); }
    virtual void convert(const IR::ToplevelBlock* block) = 0;
};

/**
This class implements a policy suitable for the SynthesizeActions pass.
The policy is: do not synthesize actions for the controls whose names
are in the specified set.
For example, we expect that the code in the deparser will not use any
tables or actions.
*/
class SkipControls : public P4::ActionSynthesisPolicy {
    // set of controls where actions are not synthesized
    const std::set<cstring> *skip;

 public:
    explicit SkipControls(const std::set<cstring> *skip) : skip(skip) { CHECK_NULL(skip); }
    bool convert(const Visitor::Context *, const IR::P4Control* control) override {
        if (skip->find(control->name) != skip->end())
            return false;
        return true;
    }
};

/**
This class implements a policy suitable for the RemoveComplexExpression pass.
The policy is: only remove complex expression for the controls whose names
are in the specified set.
For example, we expect that the code in ingress and egress will have complex
expression removed.
*/
class ProcessControls : public BMV2::RemoveComplexExpressionsPolicy {
    const std::set<cstring> *process;

 public:
    explicit ProcessControls(const std::set<cstring> *process) : process(process) {
        CHECK_NULL(process);
    }
    bool convert(const IR::P4Control* control) const {
        if (process->find(control->name) != process->end())
            return true;
        return false;
    }
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_BACKEND_H_ */
