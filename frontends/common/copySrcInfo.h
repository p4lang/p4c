/*
Copyright 2022-present VMware, Inc.

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

namespace P4 {

#ifndef FRONTENDS_COMMON_COPYSRCINFO_H_
#define FRONTENDS_COMMON_COPYSRCINFO_H_

#include "ir/ir.h"

/// This simple visitor copies the specified source information
/// to the root node of the IR tree that it is invoked on.
class CopySrcInfo : public Transform {
    const Util::SourceInfo &srcInfo;

 public:
    explicit CopySrcInfo(const Util::SourceInfo &srcInfo) : srcInfo(srcInfo) {}
    /// Only visit first node.
    const IR::Node *preorder(IR::Node *node) {
        auto result = node->clone();
        result->srcInfo = srcInfo;
        prune();
        return result;
    }
};

}  // namespace P4

#endif  // FRONTENDS_COMMON_COPYSRCINFO_H_
