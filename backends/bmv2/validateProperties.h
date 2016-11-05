/*
Copyright 2016 VMware, Inc.

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

#ifndef _BACKENDS_BMV2_VALIDATEPROPERTIES_H_
#define _BACKENDS_BMV2_VALIDATEPROPERTIES_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"

namespace BMV2 {

// Checks to see if there are any unknown properties.
class ValidateTableProperties : public Inspector {
 public:
    ValidateTableProperties()
    { setName("ValidateTableProperties"); }
    void postorder(const IR::TableProperty* property) override;
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_VALIDATEPROPERTIES_H_ */
