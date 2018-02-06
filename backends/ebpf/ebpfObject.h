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

#ifndef _BACKENDS_EBPF_EBPFOBJECT_H_
#define _BACKENDS_EBPF_EBPFOBJECT_H_

#include "target.h"
#include "ebpfModel.h"
#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "codeGen.h"

namespace EBPF {

// Base class for EBPF objects
class EBPFObject {
 public:
    virtual ~EBPFObject() {}
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const {
        return dynamic_cast<const T*>(this); }
    template<typename T> T* to() {
        return dynamic_cast<T*>(this); }

    static cstring externalName(const IR::IDeclaration* declaration) {
        cstring name = declaration->externalName();
        return name.replace('.', '_');
    }
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFOBJECT_H_ */
