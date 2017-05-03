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

#ifndef _BACKENDS_BMV2_COPYANNOTATIONS_H_
#define _BACKENDS_BMV2_COPYANNOTATIONS_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace BMV2 {

/**
 * Copy Annotations from architecture block to the corresponding
 * instance in user program.
 *
 * @pipeline
 * Control Ingress(in H h, inout M m);
 *
 * Control IngressImpl(in header h, inout metadata m);
 *
 * is converted to
 *
 * @pipeline
 * Control Ingress(in H h, inout M m);
 *
 * @pipeline
 * Control IngressImpl(in header h, inout metadata m);
 */
class CopyAnnotations : public Inspector {
    Backend* backend;
 public:
    explicit CopyAnnotations(Backend* backend) : backend(backend)
    { setName("CopyAnnotations"); }
    bool preorder(const IR::PackageBlock* block) override;
    bool preorder(const IR::P4Program* prog) override;
};

} // namespace BMV2
#endif
