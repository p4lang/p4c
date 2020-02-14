/*
Copyright 2019 Orange

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

#ifndef P4C_UBPFBACKEND_H
#define P4C_UBPFBACKEND_H

#include "backends/ebpf/ebpfOptions.h"
#include "ir/ir.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace UBPF {
    void run_ubpf_backend(const EbpfOptions &options, const IR::ToplevelBlock *toplevel,
                          P4::ReferenceMap *refMap, P4::TypeMap *typeMap);

    std::string extract_file_name(const std::string &fullPath);
} // namespace UBPF

#endif //P4C_UBPFBACKEND_H
