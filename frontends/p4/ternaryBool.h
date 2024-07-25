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

#ifndef FRONTENDS_P4_TERNARYBOOL_H_
#define FRONTENDS_P4_TERNARYBOOL_H_

#include "lib/cstring.h"

namespace P4 {
enum class TernaryBool { Yes, No, Maybe };

cstring toString(const TernaryBool &c);

}  // namespace P4

#endif /* FRONTENDS_P4_TERNARYBOOL_H_ */
