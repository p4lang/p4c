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

#ifndef _BACKENDS_BMV2_METADATA_H_
#define _BACKENDS_BMV2_METADATA_H_

#include <cstring>
#include <map>
#include <string>
#include "ir/ir.h"
#include "ir/visitor.h"

namespace Arch {

using MetadataRemapT = std::map<cstring, cstring>;
MetadataRemapT *readMap(const char *filename);

}  // namespace Arch

#if 0
// we may need this later, but for now it's handled through aliases
namespace BMV2 {
class Backend;

  /**
   * This class maps the metadata accesses (standard and intrinsic)
   * declared in the architecture to the target metadata
   */
class MetadataRemap : public Transform {
    Backend *backend;
    const Arch::MetadataRemapT *remap;

 public:
    explicit MetadataRemap(Backend *b, const Arch::MetadataRemapT *remap) :
        backend(b), remap(remap) { setName("MetadataRemap"); }

    const IR::Node * postorder(const IR::PackageBlock* b) override
    { std::cerr << b << std::endl; };
    const IR::Node * postorder(const IR::Declaration_Instance* decl) override
    { std::cerr << decl << std::endl; };
    const IR::Node * postorder(const IR::ParameterList* paramList) override
    { std::cerr << paramList << std::endl; };
    const IR::Node * postorder(const IR::P4Action* action) override
    { std::cerr << action << std::endl; };
    const IR::Node * postorder(const IR::Declaration_Variable* decl) override
    { std::cerr << decl << std::endl; };
};

}  // namespace BMV2

#endif

#endif  // _BACKENDS_BMV2_METADATA_H_

