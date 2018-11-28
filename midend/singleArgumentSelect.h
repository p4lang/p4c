/*
Copyright 2018 VMware, Inc.

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

#ifndef _MIDEND_SINGLEARGUMENTSELECT_H_
#define _MIDEND_SINGLEARGUMENTSELECT_H_

#include "ir/ir.h"

namespace P4 {

/**
   Converts select(a, b, c) into select(a ++ b ++ c).
   A similar transformation is done for the labels.
   @pre
   This should be run after SimplifySelectList and RemoveSelectBooleans.
   It assumes that all select arguments are scalar values.
*/
class SingleArgumentSelect : public Modifier {
 public:
    SingleArgumentSelect() {
        setName("SingleArgumentSelect");
    }

    bool preorder(IR::SelectCase* selCase) override;
    bool preorder(IR::SelectExpression* expression) override;
};

}  // namespace P4

#endif /* _MIDEND_SINGLEARGUMENTSELECT_H_ */
