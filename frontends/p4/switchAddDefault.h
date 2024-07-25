/*
Copyright 2020 Barefoot Networks, Inc.

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

#ifndef FRONTENDS_P4_SWITCHADDDEFAULT_H_
#define FRONTENDS_P4_SWITCHADDDEFAULT_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

/*
 * find switch statements that do not cover all possible cases in their case list (no
 * default: case and at least one action tag with no case) and add a default: {}
 * case that does nothing.
 */
class SwitchAddDefault : public Modifier {
 public:
    void postorder(IR::SwitchStatement *) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_SWITCHADDDEFAULT_H_ */
