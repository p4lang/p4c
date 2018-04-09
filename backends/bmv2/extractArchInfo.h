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

#ifndef _BACKENDS_BMV2_EXTRACTARCHINFO_H_
#define _BACKENDS_BMV2_EXTRACTARCHINFO_H_

#include "ir/ir.h"
#include "ir/visitor.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/model.h"
#include "frontends/p4/methodInstance.h"
#include "portableSwitch.h"

namespace BMV2 {

class ExtractArchInfo : public Inspector {
 private:
    P4::TypeMap*       typeMap;
    P4::PortableModel& portable_model;
 public:
    explicit ExtractArchInfo(P4::TypeMap *typeMap)
        : typeMap(typeMap), portable_model(P4::PortableModel::instance) {
        setName("ExtractArchInfo");
    }
 public:
    bool preorder(const IR::Type_Control *node) override;
    bool preorder(const IR::Type_Parser *node) override;
    bool preorder(const IR::Type_Extern *node) override;
    bool preorder(const IR::Type_Package *node) override;
    bool preorder(const IR::P4Program* program) override;
    bool preorder(const IR::Declaration_MatchKind* kind) override;
};

}  // namespace BMV2

#endif  /* _BACKENDS_BMV2_EXTRACTARCHINFO_H_ */
