/*
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

#include "typeChecker.h"

#ifdef SUPPORT_P4_14
namespace P4 {

const IR::Node *TypeInferenceBase::postorder(const IR::AttribLocal *local) {
    setType(local, local->type);
    setType(getOriginal(), local->type);
    return local;
}

}  // namespace P4
#endif
