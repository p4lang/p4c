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

#ifndef BACKENDS_EBPF_EBPFBACKEND_H_
#define BACKENDS_EBPF_EBPFBACKEND_H_

#include "ebpfObject.h"
#include "ebpfOptions.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"

namespace EBPF {

void run_ebpf_backend(const EbpfOptions &options, const IR::ToplevelBlock *toplevel,
                      P4::ReferenceMap *refMap, P4::TypeMap *typeMap);

}  // namespace EBPF

#endif /* BACKENDS_EBPF_EBPFBACKEND_H_ */
