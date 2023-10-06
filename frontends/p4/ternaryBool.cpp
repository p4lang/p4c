/*
Copyright 2022 VMware, Inc.

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

#include "ternaryBool.h"

namespace P4 {

cstring toString(const TernaryBool &c) {
    switch (c) {
        case TernaryBool::Yes:
            return "Yes";
        case TernaryBool::No:
            return "No";
            ;
        case TernaryBool::Maybe:
            return "Maybe";
    }
    return "";
}

}  // namespace P4
